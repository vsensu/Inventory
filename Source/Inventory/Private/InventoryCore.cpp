// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryCore.h"

UInventoryCore::UInventoryCore()
	: UObject(),
	slots_(nullptr),
	page_size_(Rows*Cols),
	capacity_(Pages*Rows*Cols)
{
	slots_ = new FInvSlotData[capacity_];
	for (int i = 0; i < capacity_; ++i)
	{
		free_list_.insert(i);
	}

	UE_LOG(LogTemp, Warning, TEXT("construct Inv"));
}

UInventoryCore::~UInventoryCore()
{
	delete[]slots_;
}

int UInventoryCore::LoadFrom(const FInvStore &InvStore)
{
	ClearAll();

	this->Rows = InvStore.Rows;
	this->Cols = InvStore.Cols;
	this->Pages = InvStore.Pages;
	page_size_ = Rows * Cols;
	capacity_ = page_size_ * Pages;

	if (InvStore.InvSlotData.Num() != capacity_)
		return ErrorCode::DamagedData;

	for (int i = 0; i < capacity_; ++i)
	{
		// update slots
		const auto & inv_slot_data = InvStore.InvSlotData[i];
		slots_[i].TypeId = inv_slot_data.TypeId;
		slots_[i].Num = inv_slot_data.Num;

		// update free list
		if (inv_slot_data.TypeId == EmptyItem)
		{
			free_list_.insert(i);
		}
		else
		{
			// update inv item slot
			auto item_search = inv_items_.find(inv_slot_data.TypeId);
			if (inv_items_.end() == item_search)
			{
				auto inv_item = std::make_shared<InvItemData>();
				// update total num
				inv_item->Num = inv_slot_data.Num;
				// insert index
				inv_item->Indices.insert(i);

				inv_items_[inv_slot_data.TypeId] = inv_item;
			}
			else
			{
				// update total num
				item_search->second->Num += inv_slot_data.Num;
				// insert index
				item_search->second->Indices.insert(i);
			}

			// update not full
			if (inv_slot_data.Num != MaxNumPerSlot)
			{
				auto notfull_search = notfull_.find(inv_slot_data.TypeId);
				if (notfull_.end() == notfull_search)
				{
					auto notfull_slot = std::make_shared<ItemNotFullSlot>();
					notfull_slot->insert(i);
					notfull_[inv_slot_data.TypeId] = notfull_slot;
				}
				else
				{
					notfull_search->second->insert(i);
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Inv Loaded"));
	return Success;
}

FInvStore UInventoryCore::SaveTo() const
{
	FInvStore inv_store;
	inv_store.Pages = Pages;
	inv_store.Rows = Rows;
	inv_store.Cols = Cols;
	for (int i = 0; i < capacity_; ++i)
	{
		inv_store.InvSlotData.Add(slots_[i]);
	}

	return inv_store;
}

int UInventoryCore::InSlot(int TypeId, int Num)
{
	int ecexceeded_num = Num;
	while (ecexceeded_num > 0)
	{
		int slot_index = GetItemFirstFreeSlot(TypeId);
		if (IsIndexValid(slot_index))
		{
			int used_space = slots_[slot_index].Num;
			int free_space = (MaxNumPerSlot - used_space);
			if (free_space < ecexceeded_num)
			{
				SetItemInSpecificSlot(slot_index, TypeId, MaxNumPerSlot);
				ecexceeded_num -= free_space;
			}
			else
			{
				return SetItemInSpecificSlot(slot_index, TypeId, used_space + ecexceeded_num);
			}
		}
		else
		{
			Arrange();
			slot_index = GetItemFirstFreeSlot(TypeId);
			if (!IsIndexValid(slot_index))
				return ecexceeded_num;
		}
	}

	return 0;
}

void UInventoryCore::Arrange()
{
	// clear all
	notfull_.clear();
	free_list_.clear();

	// rebuild system
	int slot_index = 0;
	for (auto item : inv_items_)
	{
		// get key information
		int item_total_num = item.second->Num;
		int item_slot_num = std::ceil((double)item_total_num / MaxNumPerSlot);
		int lastslot_itemnum = item_total_num % MaxNumPerSlot;

		// clear inv_items_
		item.second->Indices.clear();

		// rebuild slots
		for (int i = 0; i < item_slot_num; ++i)
		{
			slots_[slot_index].TypeId = item.first;
			slots_[slot_index].Num = MaxNumPerSlot;

			// rebuild inv_items_
			item.second->Indices.insert(slot_index);

			++slot_index;
		}
		// update last slot item num
		if (0 != lastslot_itemnum)
		{
			slots_[slot_index - 1].Num = lastslot_itemnum;

			// rebuild not_full_
			auto notfull_search = notfull_.find(item.first);
			if (notfull_.end() == notfull_search)
				notfull_[item.first] = std::make_shared<std::set<int>>();
			notfull_[item.first]->insert(slot_index - 1);
		}

	}

	// rebuild free slot and list
	for (int free_slot_index = slot_index; free_slot_index < capacity_; ++free_slot_index)
	{
		slots_[free_slot_index].TypeId = EmptyItem;
		slots_[free_slot_index].Num = 0;
		free_list_.insert(free_slot_index);
	}

	UpdatePageData.Broadcast();
}

int UInventoryCore::DropItemInSpecificSlot(int index)
{
	if (!IsIndexValid(index))
		return ErrorCode::InvalidIndex;

	if (IsSlotFree(index))
		return Success;

	// find the item type
	int num_in_slot = slots_[index].Num;
	int type_id = slots_[index].TypeId;

	// update inv item slot
	auto item_search = inv_items_.find(type_id);
	if (inv_items_.end() == item_search)
		return ErrorCode::InnerError;

	// update total num
	item_search->second->Num -= num_in_slot;
	// remove slot index
	auto itemindices_search = item_search->second->Indices.find(index);
	if (item_search->second->Indices.end() == itemindices_search)
		return ErrorCode::InnerError;
	item_search->second->Indices.erase(index);

	// update not full
	if (num_in_slot != MaxNumPerSlot)
	{
		auto notfull_search = notfull_.find(type_id);

		if (notfull_.end() == notfull_search)
		{
			return ErrorCode::InnerError;
		}
		else
		{
			auto notfull_indexsearch = notfull_search->second->find(index);
			if (notfull_search->second->end() == notfull_indexsearch)
				return ErrorCode::InnerError;

			notfull_search->second->erase(notfull_indexsearch);
			if (notfull_search->second->size() == 0)
				notfull_.erase(notfull_search);
		}
	}


	// update free list
	free_list_.insert(index);

	// if total num == 0 remove item
	if (item_search->second->Num == 0)
		inv_items_.erase(type_id);

	// remove slot
	slots_[index].TypeId = EmptyItem;
	slots_[index].Num = 0;

	UpdatePageData.Broadcast();

	return Success;
}

int UInventoryCore::DropItem(int TypeId, int Num)
{
	// find item
	auto item_search = inv_items_.find(TypeId);

	if (inv_items_.end() == item_search)
		return ErrorCode::InvalidTypeId;

	if (item_search->second->Num < Num || Num <= 0)
		return ErrorCode::InvalidItemNum;

	std::set<int> indices = item_search->second->Indices;

	for (auto index : indices)
	{
		if (Num >= slots_[index].Num)
		{
			// update slot
			Num -= slots_[index].Num;

			DropItemInSpecificSlot(index);

			if (0 == Num)
				break;
		}
		else
		{
			// the last
			SetItemInSpecificSlot(index, TypeId, slots_[index].Num - Num);
			break;
		}
	}

	return Success;
}

int UInventoryCore::SetItemInSpecificSlot(int index, int type_id, int num)
{
	if (!IsIndexValid(index))
		return ErrorCode::InvalidIndex;

	if (!IsItemNumValild(num))
		return ErrorCode::InvalidItemNum;

	// drop slot
	if (!IsSlotFree(index))
		DropItemInSpecificSlot(index);

	// update inv item slot
	auto item_search = inv_items_.find(type_id);

	if (inv_items_.end() == item_search)
	{
		auto inv_item = std::make_shared<InvItemData>();
		// update total num
		inv_item->Num = num;
		// insert index
		inv_item->Indices.insert(index);
		inv_items_[type_id] = inv_item;
	}
	else
	{
		// update total num
		item_search->second->Num += num;
		// insert index
		item_search->second->Indices.insert(index);
	}

	// update not full
	if (num != MaxNumPerSlot)
	{
		auto notfull_search = notfull_.find(type_id);
		if (notfull_.end() == notfull_search)
		{
			auto notfull_slot = std::make_shared<ItemNotFullSlot>();
			notfull_slot->insert(index);
			notfull_[type_id] = notfull_slot;
		}
		else
		{
			notfull_search->second->insert(index);
		}
	}

	// remove from free list
	auto free_list_search = free_list_.find(index);
	if (free_list_.end() != free_list_search)
		free_list_.erase(free_list_search);

	// update slot
	slots_[index].TypeId = type_id;
	slots_[index].Num = num;

	UpdatePageData.Broadcast();

	return Success;
}

FPageData UInventoryCore::GetPageData(int Page)
{
	UE_LOG(LogTemp, Warning, TEXT("Pages:%d Rows:%d Cols:%d PageSize:%d Capacity:%d"), Pages, Rows, Cols, page_size_, capacity_);
	int start_index = PageToIndex(Page, 0, 0);
	UE_LOG(LogTemp, Warning, TEXT("start index:%d"), start_index);
	if (!IsIndexValid(start_index))
		return FPageData();

	UE_LOG(LogTemp, Warning, TEXT("Index Valid"));
	FPageData page_data;
	for (int i = start_index; i < start_index + page_size_; ++i)
	{
		page_data.Data.Add(slots_[i]);
	}


	UE_LOG(LogTemp, Warning, TEXT("Item Num:%d"), page_data.Data.Num());
	return page_data;
}

void UInventoryCore::ClearAll()
{
	// clear all
	for (int i = 0; i < capacity_; ++i)
	{
		slots_[i].TypeId = EmptyItem;
		slots_[i].Num = 0;
	}

	notfull_.clear();
	free_list_.clear();
	inv_items_.clear();
}

int UInventoryCore::GetItemFreeSpace(int type_id) const
{
	auto notfull_search = notfull_.find(type_id);
	if (notfull_.end() == notfull_search)
		return GetFreeSlotsSpace();

	int item_slots_free_space = 0;
	for (auto notfull : *(notfull_search->second))
	{
		// 
		item_slots_free_space += MaxNumPerSlot - slots_[notfull].Num;
	}
	return item_slots_free_space + GetFreeSlotsSpace();
}

int UInventoryCore::GetItemFirstFreeSlot(int type_id) const
{
	// first find in not full
	auto notfull_search = notfull_.find(type_id);

	if (notfull_.end() == notfull_search)
		return GetFirstFreeSlot();

	return *(notfull_search->second->begin());
}
