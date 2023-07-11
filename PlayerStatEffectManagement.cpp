// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerStatEffectManagement.h"

/// @brief Adds provided stat effect to the array of currently active stat effects
/// @param inStatEffect
void UPlayerStatEffectManagement::AddStatEffect(AStatEffect *inStatEffect)
{
	if (inStatEffect != nullptr)
	{
		if (ActiveStatEffects.Find(inStatEffect) == INDEX_NONE)
		{
			UE_LOG(LogTemp, Log, TEXT("STAT ADDED %s"), *inStatEffect->GetName());
			if (inStatEffect->StatEffectCFGs)
			{
				if (inStatEffect->StatEffectCFGs->cfgs.GameplayCFG)
				{
					ALootchadorsCharacter *Owner = Cast<ALootchadorsCharacter>(GetOwner());
					// add it to the array, and call OnApply;
					ActiveStatEffects.AddUnique(inStatEffect);
					inStatEffect->OwningChar = Owner;
					inStatEffect->EffectState = EStatEffectState::Active;
					inStatEffect->OnApply(Owner);
					if (!inStatEffect->StatEffectCFGs->cfgs.GameplayCFG->Gameplay.PlayerStatTags.IsEmpty())
					{
						switch (inStatEffect->GetStatEffectType()) // enumtest
						{
						default:

							break;

						case EStatEffectType::Active:
						{
							// Convert to array
							TArray<FGameplayTag> TagArray;
							// Probably wanna get the modifier tags from the Lootchar here
							inStatEffect->StatEffectCFGs->cfgs.GameplayCFG->Gameplay.PlayerStatTags.GetGameplayTagArray(TagArray);

							FGameplayTagContainer TempTagsContainer = FGameplayTagContainer::EmptyContainer;

							// Go through each tag to see if it exists
							for (auto &Tag : TagArray)
							{
								if (RefCountedAbilityTags.Contains(Tag))
								{
									RefCountedAbilityTags[Tag] += 1;
								}
								else
								{
									RefCountedAbilityTags.Add(Tag, 1);
									TempTagsContainer.AddTag(Tag);
								}
							}
							if (TempTagsContainer != FGameplayTagContainer::EmptyContainer)
							{
								Owner->AddOrRemoveGameplayTags(TempTagsContainer, false);
							}
							break;
						}

						case EStatEffectType::Passive:
						{
							// Convert to array
							TArray<FGameplayTag> TagArray;
							// Probably wanna get the modifier tags from the Lootchar here
							inStatEffect->StatEffectCFGs->cfgs.GameplayCFG->Gameplay.PlayerStatTags.GetGameplayTagArray(TagArray);

							FGameplayTagContainer TempTagsContainer = FGameplayTagContainer::EmptyContainer;

							// Go through each tag to see if it exists
							for (auto &Tag : TagArray)
							{
								if (RefCountedAbilityTags.Contains(Tag))
								{
									int &current = RefCountedAbilityTags.FindOrAdd(Tag);
									current = current++;
								}
								else
								{
									RefCountedAbilityTags.Add(Tag, 1);
									TempTagsContainer.AddTag(Tag);
								}
							}
							if (TempTagsContainer != FGameplayTagContainer::EmptyContainer)
							{
								Owner->AddOrRemoveGameplayTags(TempTagsContainer, false);
							}
							ActiveStatEffects.AddUnique(inStatEffect);
							break;
						}
						case EStatEffectType::Container:
						{
							break;
						}
						case EStatEffectType::VisualOnly:
						{
							break;
						}
						case EStatEffectType::OneShot:
						{ // Convert to array
							TArray<FGameplayTag> TagArray;
							// Probably wanna get the modifier tags from the Lootchar here
							inStatEffect->StatEffectCFGs->cfgs.GameplayCFG->Gameplay.PlayerStatTags.GetGameplayTagArray(TagArray);

							FGameplayTagContainer TempTagsContainer = FGameplayTagContainer::EmptyContainer;

							// Go through each tag to see if it exists
							for (auto &Tag : TagArray)
							{
								RefCountedAbilityTags.Add(Tag, 1);
								TempTagsContainer.AddTag(Tag);
							}
							if (TempTagsContainer != FGameplayTagContainer::EmptyContainer)
							{
								Owner->AddOrRemoveGameplayTags(TempTagsContainer, false);
							}
							// PrintTMapValues(inStatEffect->GetName() + " : Added");
							ActiveStatEffects.AddUnique(inStatEffect);
							break;
						}
						}
					}
				}
			}
		}
	}
}

AStatEffect *UPlayerStatEffectManagement::ApplyStatEffect(FDataTableRowHandle StatCfg, FStatEffectInstanceParams InstanceParams, FTransform Transform, bool &Successful)
{
	AStatEffect *_stat = ApplyStatEffect_Impl(StatCfg, InstanceParams, Transform, Successful);
	if (_stat && Successful)
	{
		AddStatEffect(_stat);
	}
	return _stat;
}

// Queue it up...
void UPlayerStatEffectManagement::QueueStatEffect(FDataTableRowHandle StatCfg, FStatEffectInstanceParams InstanceParams, FTransform Transform, bool &Successful)
{
	AStatEffect *_stat = ApplyStatEffect_Impl(StatCfg, InstanceParams, Transform, Successful);
	if (_stat && Successful)
	{
		QueuedStatEffects.Add(_stat);
	}
}

void UPlayerStatEffectManagement::UpdateStatEffectsOnTick(float DeltaSeconds)
{
	ALootchadorsCharacter *Owner = Cast<ALootchadorsCharacter>(GetOwner());

	if (ActiveStatEffects.Num() > 0 && QueuedStatEffects.Num() > 0)
	{
		for (auto &stat : ActiveStatEffects)
		{
			if (stat != QueuedStatEffects.Last())
			{
				if (stat->GetStatEffectType() == EStatEffectType::OneShot || stat->GetStatEffectType() == EStatEffectType::Active)
				{
					stat->EffectState = EStatEffectState::NeedsKilled;
				}
			}
		}

		// COSMETIC Death loop: the stat effect is told to be removed, can call whatever it wants, but after this loop the mgr will remove it from its list and not know about it anymore.
		for (AStatEffect *StatEffect : ActiveStatEffects)
		{
			if (StatEffect->EffectState == EStatEffectState::NeedsKilled)
			{
				StatEffect->OnRemove();
				if (!StatEffect->StatEffectCFGs->cfgs.GameplayCFG->Gameplay.PlayerStatTags.IsEmpty())
				{
					// Convert to array
					TArray<FGameplayTag> TagArray;
					// Probably wanna get the modifier tags from the Lootchar here
					StatEffect->StatEffectCFGs->cfgs.GameplayCFG->Gameplay.PlayerStatTags.GetGameplayTagArray(TagArray);

					FGameplayTagContainer TempTagsContainer = FGameplayTagContainer::EmptyContainer;

					// Go through each tag to see if it exists
					for (auto &Tag : TagArray)
					{
						if (RefCountedAbilityTags.Contains(Tag))
						{
							TempTagsContainer.AddTag(Tag);
							RefCountedAbilityTags.Remove(Tag);
						}
					}
					Owner->AddOrRemoveGameplayTags(TempTagsContainer, true);
				}

				StatEffect->EffectState = EStatEffectState::Zombie;
			}
		}
	}

	if (QueuedStatEffects.Num() > 0)
	{
		AddStatEffect(QueuedStatEffects.Pop());
	}

	if (ActiveStatEffects.Num() > 0)
	{
		/*Update loop, DO NOT Delete pickups during this*/
		for (AStatEffect *StatEffect : ActiveStatEffects)
		{
			if (StatEffect->EffectState == EStatEffectState::Active)
			{
				switch (StatEffect->TimeType)
				{
				case EStatEffectTimeType::Duration:
				{
					StatEffect->OnUpdate(DeltaSeconds);
					if (StatEffect->CurrentLifetime > StatEffect->GetStatDuration())
					{
						StatEffect->EffectState = EStatEffectState::NeedsKilled;
					}
				}
				case EStatEffectTimeType::Infinite:
				{
					StatEffect->OnUpdate(DeltaSeconds);
				}
				}
			}
			StatEffect->CurrentLifetime += DeltaSeconds;
		}

		Owner = Cast<ALootchadorsCharacter>(GetOwner());
		// COSMETIC Death loop: the stat effect is told to be removed, can call whatever it wants, but after this loop the mgr will remove it from its list and not know about it anymore.
		for (AStatEffect *StatEffect : ActiveStatEffects)
		{
			if (StatEffect->EffectState == EStatEffectState::NeedsKilled)
			{
				RemoveStatEffect(StatEffect);
			}
		}

		// Housekeeping complete; now remove the dying stat FX from mgmnt
		ActiveStatEffects.RemoveAllSwap([=](AStatEffect *Stat)
										{ return Stat->EffectState == EStatEffectState::NeedsKilled || Stat->EffectState == EStatEffectState::Zombie; });
	}
} // end tick loop

/// @brief Removes stat effect / removes one from the stack of active stat effects count
/// @param inStatEffect
void UPlayerStatEffectManagement::RemoveStatEffect(AStatEffect *&inStatEffect)
{
	ALootchadorsCharacter *Owner = Cast<ALootchadorsCharacter>(GetOwner());
	// UPlayerStatEffectManagement::RemoveStatEffect(AStatEffect*& inStatEffect);
	inStatEffect->OnRemove();
	if (!inStatEffect->StatEffectCFGs->cfgs.GameplayCFG->Gameplay.PlayerStatTags.IsEmpty())
	{
		// Convert to array
		TArray<FGameplayTag> TagArray;
		// Probably wanna get the modifier tags from the Lootchar here
		inStatEffect->StatEffectCFGs->cfgs.GameplayCFG->Gameplay.PlayerStatTags.GetGameplayTagArray(TagArray);

		FGameplayTagContainer TempTagsContainer = FGameplayTagContainer::EmptyContainer;

		// Go through each tag to see if it exists
		for (auto &Tag : TagArray)
		{
			if (RefCountedAbilityTags.Contains(Tag))
			{
				RefCountedAbilityTags[Tag] -= 1;
				if (RefCountedAbilityTags[Tag] == 0)
				{
					TempTagsContainer.AddTag(Tag);
					RefCountedAbilityTags.Remove(Tag);
				}
			}
		}
		Owner->AddOrRemoveGameplayTags(TempTagsContainer, true);
	}
}
void UPlayerStatEffectManagement::PrintTMapValues(FString WhereCalledFrom)
{
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("%s"), *WhereCalledFrom));

	for (const TPair<FGameplayTag, int> &pair : RefCountedAbilityTags)
	{
		pair.Key;
		pair.Value;
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("%s %d"), *pair.Key.ToString(), pair.Value));
	}
}
