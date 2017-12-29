// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
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
