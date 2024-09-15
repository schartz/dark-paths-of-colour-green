// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

namespace Debug
{

	/*static void Print()
	{
		
	}*/

	static void Print(const FString& Msg, const FColor& Color = FColor::MakeRandomColor(), int32 InKey = -1 )
	{
		if(GEngine)
		{
			GEngine->AddOnScreenDebugMessage(InKey, 6.f, Color, Msg);
		}

		UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
	}
	
}
