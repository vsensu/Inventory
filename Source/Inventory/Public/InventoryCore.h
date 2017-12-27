// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <map>
#include <set>
#include <memory>
#include "InventoryCore.generated.h"


USTRUCT(BlueprintType)
struct FInvSlotData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Slot")
	int TypeId;

	UPROPERTY(BlueprintReadWrite, Category = "Slot")
	int Num;
	FInvSlotData() :TypeId(-1), Num(0) {}
};

USTRUCT(BlueprintType)
struct FInvStore
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Slot")
	int Pages;

	UPROPERTY(BlueprintReadWrite, Category = "Slot")
	int Rows;

	UPROPERTY(BlueprintReadWrite, Category = "Slot")
	int Cols;

	UPROPERTY(BlueprintReadWrite, Category = "Slot")
	TArray<FInvSlotData> InvSlotData;
};

struct InvItemData
{
	int Num;
	std::set<int> Indices;
	InvItemData() :Num(0) {}
};

USTRUCT(BlueprintType)
struct FPageData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	TArray<FInvSlotData> Data;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUpdatePageData);

/**
 * 
 */
UCLASS(BlueprintType)
class INVENTORY_API UInventoryCore : public UObject
{
	GENERATED_BODY()

public:
	UInventoryCore();
	~UInventoryCore();

	UFUNCTION(BlueprintCallable)
	int LoadFrom(const FInvStore &InvStore);

	UFUNCTION(BlueprintCallable)
	FInvStore SaveTo() const;

	UFUNCTION(BlueprintCallable)
	int InSlot(int TypeId, int Num);

	UFUNCTION(BlueprintCallable)
	void Arrange();

	UFUNCTION(BlueprintCallable)
	int DropItemInSpecificSlot(int index);

	UFUNCTION(BlueprintCallable)
	int DropItem(int TypeId, int Num);

	UFUNCTION(BlueprintCallable)
	int SetItemInSpecificSlot(int index, int type_id, int num);

	UFUNCTION(BlueprintCallable)
	FPageData GetPageData(int Page);

	UFUNCTION(BlueprintCallable)
	inline int GetCapacity() const { return capacity_; }


	UPROPERTY(BlueprintAssignable)
	FUpdatePageData UpdatePageData;

	UPROPERTY(BlueprintReadWrite)
	int Pages = 3;

	UPROPERTY(BlueprintReadWrite)
	int Rows = 8;

	UPROPERTY(BlueprintReadWrite)
	int Cols = 8;

	UPROPERTY(BlueprintReadWrite)
	int MaxNumPerSlot = 64;

private:
	enum { EmptyItem = -1 };
	enum { Success = 0 };
	enum ErrorCode {
		InvalidIndex = -1,
		InvalidItemNum = -2,
		InnerError = -3,	// maybe need terminate
		DamagedData = -4,
		InvalidTypeId = -5,
	};

	inline int PageToIndex(int page, int row, int col) const
	{
		if (!IsPageRowColValid(page, row, col))
			return ErrorCode::InvalidIndex;

		return page * page_size_ + row * Cols + col;
	}

	inline bool IsPageRowColValid(int page, int row, int col) const
	{
		return page >= 0 && page < Pages && row >= 0 && row < Rows && col >= 0 && col < Cols;
	}

	inline bool IsIndexValid(int index) const
	{
		return index >= 0 && index < capacity_;
	}

	inline bool HasSlotMaxLimit() const
	{
		return MaxNumPerSlot > 0;
	}

	inline bool IsItemNumValild(int num) const
	{
		if (HasSlotMaxLimit())
			return num > 0;

		return num > 0 && num <= MaxNumPerSlot;
	}

	inline bool IsSlotFree(int index) const
	{
		return slots_[index].TypeId == EmptyItem;
	}

	// not arranged
	inline int GetFreeSlotsNum() const
	{
		return free_list_.size();
	}

	// not arranged
	inline int GetFreeSlotsSpace() const
	{
		return GetFreeSlotsNum() * MaxNumPerSlot;
	}

	inline int GetFirstFreeSlot() const
	{
		if (free_list_.size() == 0)
			return InvalidIndex;

		return *free_list_.begin();
	}

	void ClearAll();

	int GetItemFreeSpace(int type_id) const;

	int GetItemFirstFreeSlot(int type_id) const;

private:
	FInvSlotData * slots_;
	std::map<int, std::shared_ptr<InvItemData>> inv_items_; // typeid itemdata

	using ItemNotFullSlot = std::set<int>;
	std::map<int, std::shared_ptr<ItemNotFullSlot>> notfull_; // typeid slotindex

	std::set<int> free_list_; // slotindex
	int page_size_;
	int capacity_;
};