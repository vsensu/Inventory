// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "InventoryCore.generated.h"


using TypeIdType = FString; // UHT not support at current

USTRUCT(BlueprintType)
struct FInvSlotData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Slot")
	FString TypeId;
	
	UPROPERTY(BlueprintReadWrite, Category = "Slot")
	int Num;

	// Invalid ID default value is empty string
	FInvSlotData() :TypeId(""), Num(0) {}
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
