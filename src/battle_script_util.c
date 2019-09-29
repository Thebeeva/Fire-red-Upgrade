#include "defines.h"
#include "defines_battle.h"
#include "../include/random.h"
#include "../include/constants/songs.h"

#include "../include/new/ability_battle_effects.h"
#include "../include/new/ability_battle_scripts.h"
#include "../include/new/ability_tables.h"
#include "../include/new/battle_start_turn_start_battle_scripts.h"
#include "../include/new/battle_util.h"
#include "../include/new/battle_script_util.h"
#include "../include/new/damage_calc.h"
#include "../include/new/daycare.h"
#include "../include/new/end_battle.h"
#include "../include/new/util.h"
#include "../include/new/item.h"
#include "../include/new/learn_move.h"
#include "../include/new/move_battle_scripts.h"
#include "../include/new/move_tables.h"
#include "../include/new/switching.h"
/*
battle_script_util.c
	general functions that aide in battle scripting via callasm.
*/

extern const u8* gBattleScriptsForMoveEffects[];

extern const species_t gTelekinesisBanList[];
extern const struct FlingStruct gFlingTable[];

void CheckIfDarkVoidShouldFail(void)
{
	if (gCurrentMove == MOVE_DARKVOID
	&&  SPECIES(gBankAttacker) != SPECIES_DARKRAI
	&&  !gNewBS->MoveBounceInProgress)
		gBattlescriptCurrInstr = BattleScript_DarkVoidFail - 5;
}

void SetTargetPartner(void)
{
	gBankTarget = PARTNER(gBankAttacker);
}

bool8 CheckCraftyShield(u8 bank)
{
	if (gSideAffecting[SIDE(bank)] & SIDE_STATUS_CRAFTY_SHIELD)
		return TRUE;

	return FALSE;
}

void LiftProtectTarget(void)
{
	if (!LiftProtect(gBankTarget))
		gBattlescriptCurrInstr = BattleScript_MoveFaint - 0x5;
}

void IncreaseNimbleCounter(void)
{
	if (gNewBS->NimbleCounters[gBankAttacker] != 0xFF)
		gNewBS->NimbleCounters[gBankAttacker] += 1;
}

void MagnetFluxLooper(void)
{
	for (; *gSeedHelper < gBattlersCount; ++*gSeedHelper)
	{
		u8 bank = gBanksByTurnOrder[*gSeedHelper];
		if ((bank == gBankAttacker || bank == PARTNER(gBankAttacker))
		&&  (ABILITY(bank) == ABILITY_PLUS || ABILITY(bank) == ABILITY_MINUS)
		&& gBattleMons[bank].hp
		&& !(gStatuses3[bank] & STATUS3_SEMI_INVULNERABLE)
		&& !(gBattleMons[bank].status2 & STATUS2_SUBSTITUTE))
		{
			++*gSeedHelper;
			gBankTarget = bank;
			if (gCurrentMove == MOVE_MAGNETICFLUX)
				gBattlescriptCurrInstr = BattleScript_MagneticFluxStatBoost - 5;
			else
				gBattlescriptCurrInstr = BattleScript_GearUpStatBoost - 5;
			return;
		}
	}

	if (!gBattleScripting->animTargetsHit) //Not a single mon was affected
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
	else
	{
		gBattlescriptCurrInstr = BattleScript_MoveEnd - 5;
		gBankTarget = gBankAttacker;
	}
}

void ModifyGrowthInSun(void)
{
	if (WEATHER_HAS_EFFECT && gBattleWeather & WEATHER_SUN_ANY)
		gBattleScripting->statChanger += INCREASE_1;
}

void AcupressureFunc(void)
{
	if (gBankAttacker != gBankTarget)
	{
		if (MoveBlockedBySubstitute(gCurrentMove, gBankAttacker, gBankTarget) || CheckCraftyShield(gBankTarget))
		{
			gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
			return;
		}
	}

	if (StatsMaxed(gBankTarget) && ABILITY(gBankTarget) != ABILITY_CONTRARY)
	{
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
		return;
	}
	else if (StatsMinned(gBankTarget) && ABILITY(gBankTarget) == ABILITY_CONTRARY)
	{
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
		return;
	}

	u8 stat;
	do
	{
		stat = umodsi(Random(), BATTLE_STATS_NO-1) + 1;
	} while (gBattleMons[gActiveBattler].statStages[stat - 1] == 12);

	SET_STATCHANGER(stat, 2, FALSE);
}

void SetStatSwapSplit(void)
{
	u8 bankAtk = gBankAttacker;
	u8 bankDef = gBankTarget;
	u32 temp, i;

	switch (gCurrentMove) {
		case MOVE_POWERTRICK:
			temp = gBattleMons[bankAtk].attack;
			gBattleMons[bankAtk].attack = gBattleMons[bankAtk].defense;
			gBattleMons[bankAtk].defense = temp;

			gStatuses3[bankAtk] ^= STATUS3_POWER_TRICK;

			gBattleStringLoader = PowerTrickString;
			break;

		case MOVE_POWERSWAP:	;
			u8 atkAtkBuff = gBattleMons[bankAtk].statStages[STAT_STAGE_ATK-1];
			u8 atkSpAtkBuff = gBattleMons[bankAtk].statStages[STAT_STAGE_SPATK-1];
			gBattleMons[bankAtk].statStages[STAT_STAGE_ATK-1] = gBattleMons[bankDef].statStages[STAT_STAGE_ATK-1];
			gBattleMons[bankAtk].statStages[STAT_STAGE_SPATK-1] = gBattleMons[bankDef].statStages[STAT_STAGE_SPATK-1];
			gBattleMons[bankDef].statStages[STAT_STAGE_ATK-1] = atkAtkBuff;
			gBattleMons[bankDef].statStages[STAT_STAGE_SPATK-1] = atkSpAtkBuff;

			gBattleStringLoader = PowerSwapString;
			break;

		case MOVE_GUARDSWAP:	;
			u8 atkDefBuff = gBattleMons[bankAtk].statStages[STAT_STAGE_DEF-1];
			u8 atkSpDefBuff = gBattleMons[bankAtk].statStages[STAT_STAGE_SPDEF-1];
			gBattleMons[bankAtk].statStages[STAT_STAGE_DEF-1] = gBattleMons[bankDef].statStages[STAT_STAGE_DEF-1];
			gBattleMons[bankAtk].statStages[STAT_STAGE_SPDEF-1] = gBattleMons[bankDef].statStages[STAT_STAGE_SPDEF-1];
			gBattleMons[bankDef].statStages[STAT_STAGE_DEF-1] = atkDefBuff;
			gBattleMons[bankDef].statStages[STAT_STAGE_SPDEF-1] = atkSpDefBuff;

			gBattleStringLoader = GuardSwapString;
			break;

		case MOVE_SPEEDSWAP:
			temp = gBattleMons[bankAtk].speed;
			gBattleMons[bankAtk].speed = gBattleMons[bankDef].speed;
			gBattleMons[bankDef].speed = temp;

			gBattleStringLoader = SpeedSwapString;
			break;

		case MOVE_HEARTSWAP:
			for (i = 0; i < BATTLE_STATS_NO-1; ++i)
			{
				temp = gBattleMons[bankAtk].statStages[i];
				gBattleMons[bankAtk].statStages[i] = gBattleMons[bankDef].statStages[i];
				gBattleMons[bankDef].statStages[i] = temp;
			}

			gBattleStringLoader = HeartSwapString;
			break;

		case MOVE_POWERSPLIT:	;
			u16 newAtk = (gBattleMons[bankAtk].attack + gBattleMons[bankDef].attack) / 2;
			u16 newSpAtk = (gBattleMons[bankAtk].spAttack + gBattleMons[bankDef].spAttack) / 2;

			gBattleMons[bankAtk].attack = MathMax(1, newAtk);
			gBattleMons[bankAtk].spAttack = MathMax(1, newSpAtk);
			gBattleMons[bankDef].attack = MathMax(1, newAtk);
			gBattleMons[bankDef].spAttack = MathMax(1, newSpAtk);

			gBattleStringLoader = PowerSplitString;
			break;

		case MOVE_GUARDSPLIT:	;
			u16 newDef = (gBattleMons[bankAtk].defense + gBattleMons[bankDef].defense) / 2;
			u16 newSpDef = (gBattleMons[bankAtk].spDefense + gBattleMons[bankDef].spDefense) / 2;

			gBattleMons[bankAtk].defense = MathMax(1, newDef);
			gBattleMons[bankAtk].spDefense = MathMax(1, newSpDef);
			gBattleMons[bankDef].defense = MathMax(1, newDef);
			gBattleMons[bankDef].spDefense = MathMax(1, newSpDef);

			gBattleStringLoader = GuardSplitString;
	}
}

void ResetTargetStats(void)
{
	for (int i = 0; i < BATTLE_STATS_NO-1; ++i)
		gBattleMons[gBankTarget].statStages[i] = 6;
}

void ToggleSpectralThiefByte(void)
{
	gNewBS->SpectralThiefActive ^= TRUE;
}

void CheeckPouchFunc(void)
{
	u8 bank = gBattleScripting->bank;
	if (ABILITY(bank) == ABILITY_CHEEKPOUCH && !gNewBS->HealBlockTimers[bank]) { //Berry check should already be done
		gBattleMoveDamage = MathMax(1, udivsi(gBattleMons[bank].maxHP, 3));
		gBattleMoveDamage *= -1;
		gFormCounter = TRUE;
	}
	else
		gFormCounter = FALSE;
}

void SetUnburdenBoostTarget(void)
{
	gNewBS->UnburdenBoosts |= 1 << gBankTarget;
}

void MoldBreakerRemoveAbilitiesOnForceSwitchIn(void)
{
	u8 bank;
	if (gForceSwitchHelper == 2)
		bank = gBattleScripting->bank;
	else
		bank = gBankAttacker;

	if (ABILITY(bank) == ABILITY_MOLDBREAKER
	||  ABILITY(bank) == ABILITY_TURBOBLAZE
	||  ABILITY(bank) == ABILITY_TERAVOLT)
	{
		if (gMoldBreakerIgnoredAbilities[ABILITY(gBankTarget)])
		{
			gNewBS->DisabledMoldBreakerAbilities[gBankTarget] = gBattleMons[gBankTarget].ability;
			gBattleMons[gBankTarget].ability = 0;
		}
	}
}


void MoldBreakerRestoreAbilitiesOnForceSwitchIn(void)
{
	if (gNewBS->DisabledMoldBreakerAbilities[gBankTarget])
	{
		gBattleMons[gBankTarget].ability = gNewBS->DisabledMoldBreakerAbilities[gBankTarget];
		gNewBS->DisabledMoldBreakerAbilities[gBankTarget] = 0;
	}
}

void TrainerSlideOut(void)
{
	gActiveBattler = B_POSITION_OPPONENT_LEFT;
	EmitTrainerSlideBack(0);
	MarkBufferBankForExecution(gActiveBattler);
}

void SetAuroraVeil(void)
{
	if (!gNewBS->AuroraVeilTimers[SIDE(gBankAttacker)]
	&& gBattleWeather & WEATHER_HAIL_ANY
	&& WEATHER_HAS_EFFECT)
	{
		if (ITEM_EFFECT(gBankAttacker) == ITEM_EFFECT_LIGHT_CLAY)
			gNewBS->AuroraVeilTimers[SIDE(gBankAttacker)] = 8;
		else
			gNewBS->AuroraVeilTimers[SIDE(gBankAttacker)] = 5;

		gBattleStringLoader = gText_SetAuroraVeil;
		gBattleCommunication[MULTISTRING_CHOOSER] = 1;
	}
	else
	{
		gMoveResultFlags |= MOVE_RESULT_MISSED;
		gBattleCommunication[MULTISTRING_CHOOSER] = 0;
	}
}

void MetalBurstDamageCalculator(void)
{
	if (GetBattlerTurnOrderNum(gBankTarget) > gCurrentTurnActionNumber)
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;

	u8 atkSide = SIDE(gBankAttacker);
	u8 defSide = SIDE(gProtectStructs[gBankAttacker].physicalBank);
	u8 defSide2 = SIDE(gProtectStructs[gBankAttacker].specialBank);

	if (gProtectStructs[gBankAttacker].physicalDmg && atkSide != defSide && gBattleMons[gProtectStructs[gBankAttacker].physicalBank].hp)
	{
		gBattleMoveDamage = (gProtectStructs[gBankAttacker].physicalDmg * 15) / 10;

		if (gSideTimers[defSide].followmeTimer && gBattleMons[gSideTimers[defSide].followmeTarget].hp)
			gBankTarget = gSideTimers[defSide].followmeTarget;
		else
			gBankTarget = gProtectStructs[gBankAttacker].physicalBank;
	}
	else if (gProtectStructs[gBankAttacker].specialDmg && atkSide != defSide2 && gBattleMons[gProtectStructs[gBankAttacker].specialBank].hp)
	{
		gBattleMoveDamage = (gProtectStructs[gBankAttacker].specialDmg * 15) / 10;

		if (gSideTimers[defSide2].followmeTimer && gBattleMons[gSideTimers[defSide2].followmeTarget].hp)
			gBankTarget = gSideTimers[defSide2].followmeTarget;
		else
			gBankTarget = gProtectStructs[gBankAttacker].specialBank;
	}
	else
	{
		gSpecialStatuses[gBankAttacker].ppNotAffectedByPressure = 1;
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
	}
}


void TryActivatePartnerSapSipper(void)
{
	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE
	&& !(gAbsentBattlerFlags & gBitTable[PARTNER(gBankAttacker)])
	&& ABILITY(PARTNER(gBankAttacker)) == ABILITY_SAPSIPPER
	&& gBattleStruct->dynamicMoveType == TYPE_GRASS
	&& STAT_CAN_RISE(PARTNER(gBankAttacker), STAT_STAGE_ATK))
	{
		gBattleScripting->bank = gBankTarget = PARTNER(gBankAttacker);
		gBattleScripting->statChanger = INCREASE_1 | STAT_STAGE_ATK;
		gBattleScripting->animArg1 = 0xE + STAT_STAGE_ATK;
		gBattleScripting->animArg2 = 0;
		gLastUsedAbility = ABILITY(gBankTarget);
		BattleScriptPush(gBattlescriptCurrInstr + 5);
		gBattlescriptCurrInstr = BattleScript_SapSipperAromatherapy - 5;
	}
}

void RoundBSFunction(void)
{
	int i;
	u8 j = 0;
	u8 k = 0;
	u8 index = 0;
	u8 rounders[3] = {0xFF, 0xFF, 0xFF};
	u8 nonRounders[3] = {0xFF, 0xFF, 0xFF};

	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE) {
		for (i = 0; i < gBattlersCount; ++i) {
			if (gBanksByTurnOrder[i] == gBankAttacker) {
				index = i + 1; //Index after attacker; index to start swapping data
				break;
			}
		}

		for (i = index; i < gBattlersCount; ++i) {
			if (gChosenMovesByBanks[gBanksByTurnOrder[i]] == MOVE_ROUND)
				rounders[j++] = gBanksByTurnOrder[i];
			else
				nonRounders[k++] = gBanksByTurnOrder[i];
		}

		for (i = 0; rounders[i] != 0xFF && i < 3; ++i)
			gBanksByTurnOrder[index++] = rounders[i];

		for (i = 0; nonRounders[i] != 0xFF && i < 3; ++i)
			gBanksByTurnOrder[index++] = nonRounders[i];

	}
}

void SetRoundUsed(void)
{
	gNewBS->roundUsed = TRUE;
}

void EchoedVoiceFunc(void)
{
	switch (gNewBS->EchoedVoiceCounter) {
		case 0:
			gNewBS->EchoedVoiceCounter = 2;
			gNewBS->EchoedVoiceDamageScale = 0;
			break;
		case 2:
			break;
		default:
			gNewBS->EchoedVoiceCounter = 2;
			gNewBS->EchoedVoiceDamageScale = MathMin(10, gNewBS->EchoedVoiceDamageScale + 1);
	}
}

void DefogHelperFunc(void)
{
	if (gNewBS->AuroraVeilTimers[SIDE(gBankTarget)]
	|| gSideAffecting[SIDE(gBankAttacker)] & SIDE_STATUS_SPIKES
	|| gSideAffecting[SIDE(gBankTarget)] & (SIDE_STATUS_SPIKES
										  | SIDE_STATUS_REFLECT
										  | SIDE_STATUS_LIGHTSCREEN
										  | SIDE_STATUS_SAFEGUARD
										  | SIDE_STATUS_MIST))
	{
		gBattlescriptCurrInstr = BattleScript_DefogAdditionalEffects - 5;
	}
}

void ClearBeakBlastBit(void)
{
	if (!(gNewBS->BeakBlastByte & gBitTable[gBankAttacker])) //Forced by Encore
		gBattlescriptCurrInstr = BattleScript_NoTargetMoveFailed - 5;
	else
		gNewBS->BeakBlastByte &= ~(gBitTable[gBankAttacker]);
}

void BestowItem(void)
{
	if (ITEM(gBankTarget) == 0
	&& ITEM(gBankAttacker) != 0
	&& CanTransferItem(SPECIES(gBankTarget), ITEM(gBankAttacker))
	&& CanTransferItem(SPECIES(gBankAttacker), ITEM(gBankAttacker)))
	{
		gLastUsedItem = gBattleMons[gBankTarget].item = ITEM(gBankAttacker);
		gBattleMons[gBankAttacker].item = 0;

		gActiveBattler = gBankAttacker;
		EmitSetMonData(0, REQUEST_HELDITEM_BATTLE, 0, 2, &gBattleMons[gActiveBattler].item);
		MarkBufferBankForExecution(gActiveBattler);

		gActiveBattler = gBankTarget;
		EmitSetMonData(0, REQUEST_HELDITEM_BATTLE, 0, 2, &gBattleMons[gActiveBattler].item);
		MarkBufferBankForExecution(gActiveBattler);
	}
	else
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
}

void BelchFunction(void)
{
	if (!(gNewBS->BelchCounters & gBitTable[gBattlerPartyIndexes[gBankAttacker]]))
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 5 - 2;
}

void StrengthSapFunc(void)
{
	u16 atk = gBattleMons[gBankTarget].attack;
	APPLY_STAT_MOD(atk, &gBattleMons[gBankTarget], atk, STAT_STAGE_ATK);
	atk = MathMax(1, atk);

	if (ITEM_EFFECT(gBankAttacker) == ITEM_EFFECT_BIG_ROOT)
		atk = udivsi(130 * atk, 100);

	gBattleMoveDamage = atk * -1;
}

void PlayAttackAnimationForExplosion(void)
{
	if (gBattleExecBuffer)
	{
		gBattlescriptCurrInstr -= 5;
		return;
	}

	if ((gHitMarker & HITMARKER_NO_ANIMATIONS))
		return;

	u8 multihit;
	gActiveBattler = gBankAttacker;
	multihit = gMultiHitCounter;

	EmitMoveAnimation(0, gCurrentMove, gBattleScripting->animTurn, gBattleMovePower, gBattleMoveDamage, gBattleMons[gBankAttacker].friendship, &gDisableStructs[gBankAttacker], multihit);
	MarkBufferBankForExecution(gBankAttacker);
}

void CopycatFunc(void)
{
	if (gNewBS->LastUsedMove == 0
	|| gNewBS->LastUsedMove == 0xFFFF
	|| CheckTableForMove(gNewBS->LastUsedMove, gCopycatBannedMoves))
	{
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 1 - 5;	//From PP Reduce
	}
	else
	{
		gHitMarker &= ~(HITMARKER_ATTACKSTRING_PRINTED);
		gRandomMove = gNewBS->LastUsedMove;
	}
}

void SetRoostFunc(void)
{
	if (gBattleMons[gBankAttacker].type1 == TYPE_FLYING
	&&  gBattleMons[gBankAttacker].type2 == TYPE_FLYING)
	{
		gNewBS->RoostCounter |= gBitTable[gBankAttacker];
		gBattleMons[gBankAttacker].type1 = TYPE_NORMAL;
		gBattleMons[gBankAttacker].type2 = TYPE_NORMAL;
	}
	else
	{
		if (gBattleMons[gBankAttacker].type1 == TYPE_FLYING)
			gBattleMons[gBankAttacker].type1 = TYPE_ROOSTLESS;

		if (gBattleMons[gBankAttacker].type2 == TYPE_FLYING)
			gBattleMons[gBankAttacker].type2 = TYPE_ROOSTLESS;
	}

	if (gBattleMons[gBankAttacker].type3 == TYPE_FLYING)
		gBattleMons[gBankAttacker].type3 = TYPE_ROOSTLESS;
}

void CaptivateFunc(void)
{
	struct Pokemon *monAttacker, *monTarget;
	u16 speciesAtk, speciesDef;
	u32 personalityAtk, personalityDef;

	if (SIDE(gBankAttacker) == B_SIDE_PLAYER)
		monAttacker = &gPlayerParty[gBattlerPartyIndexes[gBankAttacker]];
	else
		monAttacker = &gEnemyParty[gBattlerPartyIndexes[gBankAttacker]];

	if (SIDE(gBankTarget) == B_SIDE_PLAYER)
		monTarget = &gPlayerParty[gBattlerPartyIndexes[gBankTarget]];
	else
		monTarget = &gEnemyParty[gBattlerPartyIndexes[gBankTarget]];

	speciesAtk = GetMonData(monAttacker, MON_DATA_SPECIES, 0);
	personalityAtk = GetMonData(monAttacker, MON_DATA_PERSONALITY, 0);

	speciesDef = GetMonData(monTarget, MON_DATA_SPECIES, 0);
	personalityDef = GetMonData(monTarget, MON_DATA_PERSONALITY, 0);

	if (GetGenderFromSpeciesAndPersonality(speciesAtk, personalityAtk) == GetGenderFromSpeciesAndPersonality(speciesDef, personalityDef)
	|| GetGenderFromSpeciesAndPersonality(speciesAtk, personalityAtk) == MON_GENDERLESS
	|| GetGenderFromSpeciesAndPersonality(speciesDef, personalityDef) == MON_GENDERLESS)
	{
		gBattlescriptCurrInstr = BattleScript_ButItDoesntAffect - 5;
	}
}

void MeFirstFunc(void)
{
	u16 move = gChosenMovesByBanks[gBankTarget];

	if (SPLIT(move) == SPLIT_STATUS
	|| GetBattlerTurnOrderNum(gBankTarget) < gCurrentTurnActionNumber
	|| CheckTableForMove(move, gMeFirstBannedMoves)
	|| CheckTableForMove(move, gMovesThatCallOtherMoves))
	{
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
	}
	else
	{
		gNewBS->MeFirstByte = TRUE;
		gRandomMove = move;
	}

}

enum PledgeEffects
{
	Pledge_Swamp = 2,
	Pledge_SeaOfFire,
	Pledge_Rainbow,
};

void LoadPledgeScript(void)
{
	int i;
	int k = 0;

	u16 partnerMove = gChosenMovesByBanks[PARTNER(gBankAttacker)];

	if (gNewBS->PledgeHelper)
	{
		if ((gCurrentMove == MOVE_GRASSPLEDGE && partnerMove == MOVE_WATERPLEDGE)
		||  (partnerMove == MOVE_GRASSPLEDGE && gCurrentMove == MOVE_WATERPLEDGE))
		{
			gCurrentMove = MOVE_GRASSPLEDGE;
			gNewBS->PledgeHelper = Pledge_Swamp;
		}
		else if ((gCurrentMove == MOVE_GRASSPLEDGE && partnerMove == MOVE_FIREPLEDGE)
			  || (partnerMove == MOVE_GRASSPLEDGE && gCurrentMove == MOVE_FIREPLEDGE))
		{
			gCurrentMove = MOVE_FIREPLEDGE;
			gNewBS->PledgeHelper = Pledge_SeaOfFire;
		}
		else if ((gCurrentMove == MOVE_FIREPLEDGE && partnerMove == MOVE_WATERPLEDGE)
			  || (partnerMove == MOVE_FIREPLEDGE && gCurrentMove == MOVE_WATERPLEDGE))
		{
			gCurrentMove = MOVE_WATERPLEDGE;
			gNewBS->PledgeHelper = Pledge_Rainbow;
		}

		gBattlescriptCurrInstr = BattleScript_PledgeCombined - 5;
	}
	else if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE
	&& gBattleMons[PARTNER(gBankAttacker)].hp
	&& partnerMove != gCurrentMove
	&& gBattleMoves[partnerMove].effect == EFFECT_PLEDGE)
	{
		u8 index = 0;
		u8 newTurnOrder[] = {0xFF, 0xFF};

		for (i = 0; i < gBattlersCount; ++i)
		{
			if (gBanksByTurnOrder[i] == gBankAttacker)
			{
				index = i + 1; //Index after attacker; index to start swapping data
				break;
			}
		}

		for (i = index; i < gBattlersCount; ++i) {
			if (gBanksByTurnOrder[i] != PARTNER(gBankAttacker))
				newTurnOrder[k++] = gBanksByTurnOrder[i];
		}

		gBanksByTurnOrder[index++] = PARTNER(gBankAttacker);

		if (newTurnOrder[0] != 0xFF)
			gBanksByTurnOrder[index++] = newTurnOrder[0];

		if (newTurnOrder[1] != 0xFF)
			gBanksByTurnOrder[index] = newTurnOrder[1];

		gNewBS->PledgeHelper = TRUE;
		gBattleScripting->bank = PARTNER(gBankAttacker);
		gBattlescriptCurrInstr = BattleScript_PledgeReady - 5;
	}
}

void SetPledgeEffect(void)
{
	switch (gNewBS->PledgeHelper) {
		case Pledge_Swamp:
			if (!BankSideHasSwamp(gBankTarget))
			{
				gNewBS->SwampTimers[SIDE(gBankTarget)] = 5;
				gBattleStringLoader = SwampString;
				//gBattleScripting->animArg1 = B_ANIM_SWAMP;
			}
			else
				gBattlescriptCurrInstr = BattleScript_MoveFaint - 5;
			break;

		case Pledge_SeaOfFire:
			if (!BankSideHasSeaOfFire(gBankTarget))
			{
				gNewBS->SeaOfFireTimers[SIDE(gBankTarget)] = 5;
				gBattleStringLoader = SeaOfFireString;
				gBattleScripting->animArg1 = B_ANIM_SEA_OF_FIRE;
			}
			else
				gBattlescriptCurrInstr = BattleScript_MoveFaint - 5;
			break;

		case Pledge_Rainbow:
			if (!BankSideHasRainbow(gBankTarget))
			{
				gNewBS->RainbowTimers[SIDE(gBankTarget)] = 5;
				gBattleStringLoader = RainbowString;
				//BattleScripting->animArg1 = B_ANIM_RAINBOW;
			}
			else
				gBattlescriptCurrInstr = BattleScript_MoveFaint - 5;
			break;

		default:
			gBattlescriptCurrInstr = BattleScript_MoveFaint - 5;
	}

	gNewBS->PledgeHelper = 0;
}

void DoFieldEffect(void)
{
	switch (gCurrentMove) {
		case MOVE_TRICKROOM:
			if (gNewBS->TrickRoomTimer > 0)
			{
				gNewBS->TrickRoomTimer = 0;
				gBattleStringLoader = TrickRoomEndString;
			}
			else if (!IsTrickRoomActive())
			{
				gNewBS->TrickRoomTimer = 5;
				gBattleStringLoader = TrickRoomSetString;
			}
			else
				 gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
			break;

		case MOVE_WONDERROOM:
			if (gNewBS->WonderRoomTimer > 0)
			{
				gNewBS->WonderRoomTimer = 0;
				gBattleStringLoader = WonderRoomEndString;
			}
			else if (!IsWonderRoomActive())
			{
				gNewBS->WonderRoomTimer = 5;
				gBattleStringLoader = WonderRoomSetString;
			}
			else
				 gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
			break;

		case MOVE_MAGICROOM:
			if (gNewBS->MagicRoomTimer)
			{
				gNewBS->MagicRoomTimer = 0;
				gBattleStringLoader = MagicRoomEndString;
			}
			else if (!IsMagicRoomActive())
			{
				gNewBS->MagicRoomTimer = 5;
				gBattleStringLoader = MagicRoomSetString;
			}
			else
				 gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
			break;

		case MOVE_GRAVITY:
			if (gNewBS->GravityTimer > 0)
			{
				gNewBS->GravityTimer = 0;
				gBattleStringLoader = GravityEndString;
			}
			else if (!IsGravityActive())
			{
				for (int i = 0; i < gBattlersCount; ++i)
				{
					if (!CheckGrounding(i))
						gNewBS->targetsToBringDown |= gBitTable[i];
				}

				gNewBS->GravityTimer = 5;
				gBattleStringLoader = GravitySetString;
			}
			else
				 gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
			break;

		case MOVE_IONDELUGE:
			if (!IsIonDelugeActive())
			{
				gNewBS->IonDelugeTimer = 1;
			}

			//Doesn't fail even if already Ion Deluge
			gBattleStringLoader = IonDelugeShowerString;
			break;
	}
}

//Continues until no marked mons are left in air
void BringDownMons(void)
{
	for (int i = 0; i < gBattlersCount; ++i)
	{
		if (gNewBS->targetsToBringDown & gBitTable[i])
		{
			if (gStatuses3[i] & (STATUS3_IN_AIR | STATUS3_SKY_DROP_ATTACKER))
			{
				gNewBS->NoMoreMovingThisTurn |= gBitTable[i];
				CancelMultiTurnMoves(i);
			}

			gNewBS->targetsToBringDown ^= gBitTable[i];
			gNewBS->TelekinesisTimers[i] = 0;
			gNewBS->MagnetRiseTimers[i] = 0;
			gStatuses3[i] &= ~(STATUS3_IN_AIR
							 | STATUS3_SKY_DROP_ATTACKER
							 | STATUS3_SKY_DROP_TARGET
							 | STATUS3_LEVITATING
							 | STATUS3_TELEKINESIS);

			gActiveBattler = i;
			EmitSpriteInvisibility(0, FALSE);
			MarkBufferBankForExecution(gActiveBattler);

			gBattleScripting->bank = i;
			if (gCurrentMove == MOVE_GRAVITY)
				gBattleStringLoader = NoMoreAirborneFromGravityString;
			else
				gBattleStringLoader = Bank0AWasBroughtDownString;
			BattleScriptPushCursor();
			gBattlescriptCurrInstr = BattleScript_PrintCustomString - 5;
			return;
		}
	}
}

void TryFling(void)
{
	if (!CanFling(ITEM(gBankAttacker), SPECIES(gBankAttacker), ABILITY(gBankAttacker),
				  gBankAttacker, gNewBS->EmbargoTimers[gBankAttacker]))
	{
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
	}
	else
	{
		gLastUsedItem = ITEM(gBankAttacker);
		gBattleCommunication[MOVE_EFFECT_BYTE] = gFlingTable[gLastUsedItem].effect;
	}
}

void CheckTelekinesisFail(void)
{
	u16 species = gBattleMons[gBankTarget].species;

	if (gStatuses3[gBankTarget] & (STATUS3_TELEKINESIS | STATUS3_ROOTED | STATUS3_SMACKED_DOWN)
	||  IsGravityActive()
	||  ITEM_EFFECT(gBankTarget) == ITEM_EFFECT_IRON_BALL
	||  CheckTableForSpecies(species, gTelekinesisBanList))
	{
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 5 - 2;
	}
}

void ChangeTargetTypeFunc(void)
{
	switch (gCurrentMove) {
		case MOVE_SOAK:
			if (ABILITY(gBankTarget) == ABILITY_MULTITYPE
			||  ABILITY(gBankTarget) == ABILITY_RKS_SYSTEM
			|| (gBattleMons[gBankTarget].type1 == TYPE_WATER &&
				gBattleMons[gBankTarget].type2 == TYPE_WATER &&
				gBattleMons[gBankTarget].type3 == TYPE_BLANK))
			{
				gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
			}
			else
			{
				SET_BATTLER_TYPE(gBankTarget, TYPE_WATER);
				PREPARE_TYPE_BUFFER(gBattleTextBuff1, TYPE_WATER);
				gBattleStringLoader = TargetTransformedIntoType;
			}
			break;

		case MOVE_TRICKORTREAT:
			if (IsOfType(gBankTarget, TYPE_GHOST))
				gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
			else
			{
				if (gBattleMons[gBankTarget].type1 == gBattleMons[gBankTarget].type2)
					gBattleMons[gBankTarget].type2 = TYPE_GHOST;
				else
					gBattleMons[gBankTarget].type3 = TYPE_GHOST;

				PREPARE_TYPE_BUFFER(gBattleTextBuff1, TYPE_GHOST);
				gBattleStringLoader = ThirdTypeAddedString;
			}
			break;

		case MOVE_FORESTSCURSE:
			if (IsOfType(gBankTarget, TYPE_GRASS))
				gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
			else
			{
				if (gBattleMons[gBankTarget].type1 == gBattleMons[gBankTarget].type2)
					gBattleMons[gBankTarget].type2 = TYPE_GRASS;
				else
					gBattleMons[gBankTarget].type3 = TYPE_GRASS;

				PREPARE_TYPE_BUFFER(gBattleTextBuff1, TYPE_GRASS);
				gBattleStringLoader = ThirdTypeAddedString;
			}
			break;
	}
}

void HealTargetFunc(void)
{
	u16 maxHP = gBattleMons[gBankTarget].maxHP;

	if (BATTLER_MAX_HP(gBankTarget)) {
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
		return;
	}

	gBattleMoveDamage = 0;

	switch (gCurrentMove) {
		case MOVE_HEALPULSE:
			if (ABILITY(gBankTarget) == ABILITY_MEGALAUNCHER)
				gBattleMoveDamage = udivsi(maxHP * 75, 100);
			else
				gBattleMoveDamage = maxHP / 2;
			break;

		case MOVE_FLORALHEALING:
			if (gTerrainType == GRASSY_TERRAIN)
				gBattleMoveDamage = udivsi(maxHP * 2, 3);
			else
				gBattleMoveDamage = maxHP / 2;
			break;

		case MOVE_POLLENPUFF:
			gBattleMoveDamage = maxHP / 2;
			break;
	}

	gBattleMoveDamage = MathMax(gBattleMoveDamage, 1);

	gBattleMoveDamage *= -1;
}

void TopsyTurvyFunc(void)
{
	int i;

	for (i = 0; i < BATTLE_STATS_NO-1; ++i)
	{
		if (gBattleMons[gBankTarget].statStages[i] != 6)
			break;
	}

	if (i == BATTLE_STATS_NO-1)
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
	else
	{
		for (i = 0; i < BATTLE_STATS_NO-1; ++i)
			gBattleMons[gBankTarget].statStages[i] = ((gBattleMons[gBankTarget].statStages[i] - 6) * -1) + 6;
	}
}

void DoFairyLockHappyHourFunc(void)
{
	switch (gCurrentMove) {
		case MOVE_FAIRYLOCK:
			if (IsFairyLockActive())
				gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
			else
			{
				gNewBS->FairyLockTimer = 2;
				gBattleStringLoader = FairyLockString;
			}
			break;

		case MOVE_HAPPYHOUR:
			if (!gNewBS->HappyHourByte)
			{
				gNewBS->HappyHourByte = TRUE;
				gBattleStruct->moneyMultiplier *= 2;
			}
			gBattleStringLoader = HappyHourString;
			break;

		case MOVE_CELEBRATE:
			gBattleStringLoader = CelebrateString;
			break;

		case MOVE_HOLDHANDS:
			if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE
			&&  gBattleMons[PARTNER(gBankAttacker)].hp)
			{
				gBattleStringLoader = HoldHandsString;
			}
			else
				gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;

			break;
	}
}

bool8 TargetMovesRightAfterAttacker(void)
{
	for (int i = 1; i < gBattlersCount; ++i)
	{
		if (gBanksByTurnOrder[i] == gBankTarget
		&&  gBanksByTurnOrder[i - 1] == gBankAttacker)
			return TRUE;
	}
	return FALSE;
}

void AfterYouFunc(void)
{
	int i;

	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE
	&&  GetBattlerTurnOrderNum(gBankTarget) > gCurrentTurnActionNumber //Attacker moved before target
	&&  !TargetMovesRightAfterAttacker())
	{
		u8 index = 0;
		u8 k = 0;
		u8 newTurnOrder[] = {0xFF, 0xFF};

		for (i = 0; i < gBattlersCount; ++i)
		{
			if (gBanksByTurnOrder[i] == gBankAttacker)
			{
				index = i + 1; //Index after attacker; index to start swapping data
				break;
			}
		}

		for (i = index; i < gBattlersCount; ++i)
		{
			if (gBanksByTurnOrder[i] != gBankTarget)
				newTurnOrder[k++] = gBanksByTurnOrder[i];
		}

		gBanksByTurnOrder[index++] = gBankTarget;

		if (newTurnOrder[0] != 0xFF)
			gBanksByTurnOrder[index++] = newTurnOrder[0];

		if (newTurnOrder[1] != 0xFF)
			gBanksByTurnOrder[index] = newTurnOrder[1];
	}
	else
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
}

void QuashFunc(void)
{
	int i;

	u8 numPokemonOnField = CountAliveMonsInBattle(BATTLE_ALIVE_ATK_SIDE, gBankAttacker, gBankTarget)
						 + CountAliveMonsInBattle(BATTLE_ALIVE_DEF_SIDE, gBankAttacker, gBankTarget);

	if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE
	&&  GetBattlerTurnOrderNum(gBankTarget) > gCurrentTurnActionNumber) //Attacker moved before target
	{
		int k = 0;
		u8 newTurnOrder[] = {0xFF, 0xFF, 0xFF, 0xFF};

		for (i = 0; i < numPokemonOnField; ++i)
		{
			if (gBanksByTurnOrder[i] != gBankTarget)
				newTurnOrder[k++] = gBanksByTurnOrder[i];
		}

		newTurnOrder[k] = gBankTarget;

		for (i = 0; i < numPokemonOnField; ++i)
		{
			if (newTurnOrder[i] != 0xFF)
				gBanksByTurnOrder[i] = newTurnOrder[i];
		}
	}
	else
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
}

void TryExecuteInstruct(void)
{
	u16 move = gLastPrintedMoves[gBankTarget];

	if (CheckTableForMove(move, gInstructBannedMoves)
	||  CheckTableForMove(move, gMovesThatRequireRecharging)
	||  CheckTableForMove(move, gMovesThatCallOtherMoves)
	|| (IsZMove(move))
	|| (gLockedMoves[gBankTarget] != 0 && gLockedMoves[gBankTarget] != 0xFFFF)
	|| gBattleMons[gBankTarget].status2 & STATUS2_MULTIPLETURNS
	|| FindMovePositionInMoveset(move, gBankTarget) == 4 //No longer knows the move
	|| gBattleMons[gBankTarget].pp[FindMovePositionInMoveset(move, gBankTarget)] == 0
	|| (GetBattlerTurnOrderNum(gBankTarget) > gCurrentTurnActionNumber && (gChosenMovesByBanks[gBankTarget] == MOVE_FOCUSPUNCH
													 || gChosenMovesByBanks[gBankTarget] == MOVE_BEAKBLAST
													 || gChosenMovesByBanks[gBankTarget] == MOVE_SHELLTRAP)))
	{
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
		return;
	}
}

void InitiateInstruct(void)
{
	gCurrentMove = gLastPrintedMoves[gBankTarget];
	gCurrMovePos = FindMovePositionInMoveset(gCurrentMove, gBankTarget);
	gChosenMovePos = FindMovePositionInMoveset(gCurrentMove, gBankTarget);

	gBankAttacker = gBankTarget;

	if (gAbsentBattlerFlags & gBitTable[gNewBS->lastTargeted[gBankAttacker]])
		gBankTarget = GetMoveTarget(gCurrentMove, FALSE);
	else
		gBankTarget = gNewBS->lastTargeted[gBankAttacker];

	gLastUsedMoves[gBankAttacker] = gChosenMove;
	gLastResultingMoves[gBankAttacker] = gCurrentMove;
	gLastPrintedMoves[gBankAttacker] = gCurrentMove;

	gBattleScripting->animTargetsHit = 0;
	gBattleStruct->atkCancellerTracker = 0;
	MoveValuesCleanUp();
	gHitMarker &= ~(HITMARKER_ATTACKSTRING_PRINTED);
	BattleScriptPush(gBattleScriptsForMoveEffects[gBattleMoves[gCurrentMove].effect]);
	gBattlescriptCurrInstr = BattleScript_FlushMessageBox - 5;
	gNewBS->ZMoveData->active = FALSE;
	gNewBS->InstructInProgress = TRUE;
}

void TrySetMagnetRise(void)
{
	if (IsGravityActive()
	|| gNewBS->MagnetRiseTimers[gBankAttacker] != 0
	|| gStatuses3[gBankAttacker] & (STATUS3_LEVITATING | STATUS3_SMACKED_DOWN | STATUS3_ROOTED)
	|| ITEM_EFFECT(gBankAttacker) == ITEM_EFFECT_IRON_BALL)
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
	else
	{
		gNewBS->MagnetRiseTimers[gBankAttacker] = 5;
		gStatuses3[gBankAttacker] |= STATUS3_LEVITATING;
	}
}

void TailwindLuckyChantFunc(void)
{
	switch (gCurrentMove) {
		case MOVE_TAILWIND:
			if (gNewBS->TailwindTimers[SIDE(gBankAttacker)])
				gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
			else
			{
				gNewBS->TailwindTimers[SIDE(gBankAttacker)] = 4;
				gBattleStringLoader = TailwindSetString;
			}
			break;
		case MOVE_LUCKYCHANT:
			if (gNewBS->LuckyChantTimers[SIDE(gBankAttacker)])
				gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
			else
			{
				gNewBS->LuckyChantTimers[SIDE(gBankAttacker)] = 4;
				gBattleStringLoader = LuckyChantSetString;
			}
			break;
	}
}

void FlameBurstFunc(void)
{
	u8 par = PARTNER(gBankTarget);

	if (!(gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
	||  ABILITY(par) == ABILITY_MAGICGUARD
	||  gBattleMons[par].hp == 0)
	{
		gBattlescriptCurrInstr = BattleScript_MoveEnd - 5;
	}
	else
	{
		gBattleScripting->bank = par;
		gBattleMoveDamage = MathMax(1, gBattleMons[par].maxHP / 16);
	}
}

void ModifyPostStockpileBoostDecrement(void)
{
	switch (gDisableStructs[gBankAttacker].stockpileCounter) {
		case 1:
			gBattleScripting->statChanger |= DECREASE_1;
			break;
		case 2:
			gBattleScripting->statChanger |= DECREASE_2;
			break;
		case 3:
			gBattleScripting->statChanger |= DECREASE_3;
			break;
	}
}

void RemoveStockpileBoosts(void)
{
	gDisableStructs[gBankAttacker].stockpileCounter = 0;
}

void SetHealingWishLunarDanceFunc(void)
{
	switch (gCurrentMove) {
		case MOVE_HEALINGWISH:
			gNewBS->HealingWishLoc |= gBitTable[gBankAttacker];
			break;
		case MOVE_LUNARDANCE:
			gNewBS->HealingWishLoc |= (gBitTable[gBankAttacker] << 4);
	}
}

void FinalGambitDamageCalc(void)
{
	gBattleMoveDamage = gBattleMons[gBankAttacker].hp;
}

void AbilityChangeBSFunc(void)
{
	if (gBattleExecBuffer)
	{
		gBattlescriptCurrInstr -= 5;
		return;
	}

	u8* atkAbilityLoc, *defAbilityLoc;
	u8 atkAbility, defAbility;

	//Get correct location of ability
	atkAbilityLoc = GetAbilityLocation(gBankAttacker);
	defAbilityLoc = GetAbilityLocation(gBankTarget);

	atkAbility = *atkAbilityLoc;
	defAbility = *defAbilityLoc;

	gNewBS->backupAbility = *defAbilityLoc;

	switch (gCurrentMove) {
		case MOVE_WORRYSEED:
			if (CheckTableForAbility(defAbility, gWorrySeedGastroAcidBannedAbilities))
				gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
			else
			{
				*defAbilityLoc = ABILITY_INSOMNIA;
				gLastUsedAbility = defAbility; //Original ability
				gNewBS->SlowStartTimers[gBankTarget] = 0;
				gBattleStringLoader = WorrySeedString;
			}
			break;

		case MOVE_GASTROACID:
			if (CheckTableForAbility(defAbility, gWorrySeedGastroAcidBannedAbilities)
			|| gStatuses3[gBankTarget] & STATUS3_ABILITY_SUPPRESS)
			{
				gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
			}
			else
			{
				gStatuses3[gBankTarget] |= STATUS3_ABILITY_SUPPRESS;
				gNewBS->SuppressedAbilities[gBankTarget] = defAbility;
				*defAbilityLoc = 0;
				gNewBS->SlowStartTimers[gBankTarget] = 0;
				gBattleScripting->bank = gBankTarget;
				gBattleStringLoader = AbilitySuppressedString;
				return; //No transfer needed
			}
			break;

		case MOVE_ENTRAINMENT:
			if (atkAbility == ABILITY_NONE
			||  CheckTableForAbility(atkAbility, gEntrainmentBannedAbilitiesAttacker)
			||  CheckTableForAbility(defAbility, gEntrainmentBannedAbilitiesTarget))
				gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
			else
			{
				*defAbilityLoc = atkAbility;
				gLastUsedAbility = defAbility; //Original ability
				gNewBS->SlowStartTimers[gBankTarget] = 0;
				gBattleStringLoader = EntrainmentString;

				if (gLastUsedAbility == ABILITY_TRUANT)
					gDisableStructs[gBankTarget].truantCounter = 0; //Reset counter
			}
			break;

		case MOVE_SIMPLEBEAM:
			if (CheckTableForAbility(defAbility, gSimpleBeamBannedAbilities))
			{
				gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
			}
			else
			{
				*defAbilityLoc = ABILITY_SIMPLE;
				gLastUsedAbility = defAbility; //Original ability
				gNewBS->SlowStartTimers[gBankTarget] = 0;
				gBattleStringLoader = SimpleBeamString;
			}
			break;
	}

	if (gBattlescriptCurrInstr != BattleScript_ButItFailed - 5)
		TransferAbilityPopUp(gBankTarget, gLastUsedAbility);
}

void LoadStatustoPsychoShiftTransfer(void)
{
	u32 status = gBattleMons[gBankAttacker].status1;

	if (status & STATUS_SLEEP && CanBePutToSleep(gBankTarget, TRUE))
	{
		gBattleCommunication[MOVE_EFFECT_BYTE] = MOVE_EFFECT_SLEEP;
	}
	else if (status & STATUS_TOXIC_POISON && CanBePoisoned(gBankTarget, gBankAttacker, TRUE))
	{
		gBattleCommunication[MOVE_EFFECT_BYTE] = MOVE_EFFECT_TOXIC;
	}
	else if (status & STATUS_POISON && CanBePoisoned(gBankTarget, gBankAttacker, TRUE))
	{
		gBattleCommunication[MOVE_EFFECT_BYTE] = MOVE_EFFECT_POISON;
	}
	else if (status & STATUS_BURN && CanBeBurned(gBankTarget, TRUE))
	{
		gBattleCommunication[MOVE_EFFECT_BYTE] = MOVE_EFFECT_BURN;
	}
	else if (status & STATUS_PARALYSIS && CanBeParalyzed(gBankTarget, TRUE))
	{
		gBattleCommunication[MOVE_EFFECT_BYTE] = MOVE_EFFECT_PARALYSIS;
	}
	else
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 5;
}

void PluckBerryEat(void)
{
	gBattlescriptCurrInstr += 5;

	if (ItemBattleEffects(ItemEffects_EndTurn, gBankAttacker, TRUE, TRUE))
		gBattlescriptCurrInstr -= 5;
	else if (ItemBattleEffects(ItemEffects_ContactTarget, gBankAttacker, TRUE, TRUE))
		gBattlescriptCurrInstr -= 5;
	else
		gBattlescriptCurrInstr -= 5;
}

void BurnUpFunc(void)
{
	if (gBattleMons[gBankAttacker].type1 == TYPE_FIRE)
		gBattleMons[gBankAttacker].type1 = TYPE_MYSTERY;

	if (gBattleMons[gBankAttacker].type2 == TYPE_FIRE)
		gBattleMons[gBankAttacker].type2 = TYPE_MYSTERY;

	if (gBattleMons[gBankAttacker].type3 == TYPE_FIRE)
		gBattleMons[gBankAttacker].type3 = TYPE_BLANK;
}

void SeedRoomServiceLooper(void)
{
	for (; *gSeedHelper < gBattlersCount; ++*gSeedHelper)
	{
		u8 bank = gBanksByTurnOrder[*gSeedHelper];

		if (ITEM_EFFECT(bank) == ITEM_EFFECT_SEEDS || ITEM_EFFECT(bank) == ITEM_EFFECT_ROOM_SERVICE)
		{
			if (ItemBattleEffects(ItemEffects_SwitchIn, bank, TRUE, FALSE))
			{
				gBattlescriptCurrInstr -= 5;
				return;
			}
		}
	}
}

void LastResortFunc(void)
{
	if (!CanUseLastResort(gBankAttacker))
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 2 - 5;
}

bool8 CanUseLastResort(u8 bank)
{
	u32 i;
	bool8 knowsLastResort = FALSE;

	for (i = 0; i < MAX_MON_MOVES && gBattleMons[bank].moves[i] != MOVE_NONE; ++i)
	{
		u16 move = gBattleMons[bank].moves[i];

		if (move == MOVE_LASTRESORT)
			knowsLastResort = TRUE; //Last Resort can't be called from other moves

		else if (!(gNewBS->usedMoveIndices[bank] & gBitTable[i]))
			return FALSE;
	}

	if (i == 1 //Attacker only knows Last Resort
	|| !knowsLastResort)
		return FALSE;

	return TRUE;
}

void SynchronoiseFunc(void)
{
	if (ITEM_EFFECT(gBankTarget) == ITEM_EFFECT_RING_TARGET
	|| (!IS_BLANK_TYPE(gBattleMons[gBankAttacker].type1) && IsOfType(gBankTarget, gBattleMons[gBankAttacker].type1))
	|| (!IS_BLANK_TYPE(gBattleMons[gBankAttacker].type2) && IsOfType(gBankTarget, gBattleMons[gBankAttacker].type2))
	|| (!IS_BLANK_TYPE(gBattleMons[gBankAttacker].type3) && IsOfType(gBankTarget, gBattleMons[gBankAttacker].type3)))
	{
		return;
	}

	gBattlescriptCurrInstr = BattleScript_ButItDoesntAffect - 5;
}

void MakeScriptingBankInvisible(void)
{
	if (gBattleExecBuffer) return;

	gActiveBattler = gBattleScripting->bank;
	EmitSpriteInvisibility(0, TRUE);
	MarkBufferBankForExecution(gActiveBattler);
}

void TransferTerrainData(void)
{
	if (gBattleExecBuffer)
		gBattlescriptCurrInstr -= 5;
	else
	{
		gActiveBattler = 0;
		EmitDataTransfer(0, &gTerrainType, 1, &gTerrainType);
		MarkBufferBankForExecution(gActiveBattler);
	}
}

void TransferIllusionBroken(void)
{
	if (gBattleExecBuffer)
	{
		gBattlescriptCurrInstr -= 5;
		return;
	}

	gActiveBattler = gBattleScripting->bank;
	EmitDataTransfer(0, &gStatuses3[gActiveBattler], 4, &gStatuses3[gActiveBattler]);
	MarkBufferBankForExecution(gActiveBattler);
}

void CycleScriptingBankHealthBetween0And1(void)
{
	if (gBattleExecBuffer)
	{
		gBattlescriptCurrInstr -= 5;
		return;
	}

	gActiveBattler = gBattleScripting->bank;

	if (!gBattleMons[gActiveBattler].hp)
		gBattleMons[gActiveBattler].hp = 1;
	else
		gBattleMons[gActiveBattler].hp = 0;

	EmitSetMonData(0, REQUEST_HP_BATTLE, 0, 2, &gBattleMons[gActiveBattler].hp);
	MarkBufferBankForExecution(gActiveBattler);
}

void SetTeleportBit(void)
{
	gNewBS->TeleportBit = TRUE;
}

void TryLaunchShellTrap(void)
{
	u8 atkSide = SIDE(gBankAttacker);
	u8 defSide = SIDE(gProtectStructs[gBankAttacker].physicalBank);

	if (!gProtectStructs[gBankAttacker].physicalDmg || atkSide == defSide)
		gBattlescriptCurrInstr = BattleScript_ButItFailed - 5 - 2;
}

void TrySuckerPunch(void)
{
	if ((SPLIT(gChosenMovesByBanks[gBankTarget]) != SPLIT_STATUS && GetBattlerTurnOrderNum(gBankTarget) > gCurrentTurnActionNumber) //Attacker moved before target
	|| gChosenMovesByBanks[gBankTarget] == MOVE_MEFIRST)
		return;

	gBattlescriptCurrInstr = BattleScript_ButItFailed - 5 - 2;
}

void DoProteanTypeChange(void)
{
	u8 moveType = gBattleStruct->dynamicMoveType;

	gBattleScripting->bank = gBankAttacker;
	SET_BATTLER_TYPE(gBankAttacker, moveType);
	PREPARE_TYPE_BUFFER(gBattleTextBuff1, moveType);
}

void HarvestActivateBerry(void)
{
	gBattlescriptCurrInstr += 5;
	if (ItemBattleEffects(ItemEffects_EndTurn, gBattleScripting->bank, TRUE, FALSE))
		gBattlescriptCurrInstr -= 5;
}

void TryManipulateDamageForLeechSeedBigRoot(void)
{
	if (ITEM_EFFECT(gBankTarget) == ITEM_EFFECT_BIG_ROOT)
		gBattleMoveDamage = (gBattleMoveDamage * 130) / 100;
}

#define gText_BattleYesNoChoice (u8*) 0x83FE791
void DisplayForfeitYesNoBox(void)
{
	if (gBattleExecBuffer)
	{
		gBattlescriptCurrInstr -= 5;
		return;
	}

	HandleBattleWindow(0x17, 8, 0x1D, 0xD, 0);
	BattlePutTextOnWindow(gText_BattleYesNoChoice, 0xE);
	gBattleCommunication[CURSOR_POSITION] = 0;
	BattleCreateYesNoCursorAt(0);
}

void HandleForfeitYesNoBox(void)
{
	if (gMain.newKeys & DPAD_UP && gBattleCommunication[CURSOR_POSITION] != 0)
	{
		PlaySE(SE_SELECT);
		BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
		gBattleCommunication[CURSOR_POSITION] = 0;
		BattleCreateYesNoCursorAt(0);
	}
	if (gMain.newKeys & DPAD_DOWN && gBattleCommunication[CURSOR_POSITION] == 0)
	{
		PlaySE(SE_SELECT);
		BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
		gBattleCommunication[CURSOR_POSITION] = 1;
		BattleCreateYesNoCursorAt(1);
	}
	if (gMain.newKeys & A_BUTTON)
	{
		PlaySE(SE_SELECT);

		if (gBattleCommunication[1] == 0)
			gBattleMainFunc = (u32) HandleEndTurn_RanFromBattle;

		HandleBattleWindow(0x17, 0x8, 0x1D, 0xD, WINDOW_CLEAR);
		return;
	}
	else if (gMain.newKeys & B_BUTTON)
	{
		PlaySE(SE_SELECT);
		HandleBattleWindow(0x17, 0x8, 0x1D, 0xD, WINDOW_CLEAR);
		return;
	}

	gBattlescriptCurrInstr -= 5;
}

void BadDreamsHurtFunc(void)
{
	gBattleMoveDamage = MathMax(1, gBattleMons[gBankTarget].maxHP / 8);
}

void GrassyTerrainHealFunc(void)
{
	gBattleMoveDamage = -1 * MathMax(1, gBattleMons[gBankAttacker].maxHP / 16);
}

void RestoreBanksFromSynchronize(void)
{
	gBankAttacker = gNewBS->backupSynchronizeBanks[0];
	gBankTarget = gNewBS->backupSynchronizeBanks[1];
};

void TrySetAlternateFlingEffect(void)
{
	u8 effect = ItemId_GetHoldEffect(ITEM(gBankAttacker));

	switch (effect) {
		case ITEM_EFFECT_CURE_ATTRACT:
		case ITEM_EFFECT_RESTORE_STATS:
			if (ItemBattleEffects(ItemEffects_EndTurn, gBankTarget, TRUE, TRUE))
				gBattlescriptCurrInstr -= 5;
	}
}

void TransferLastUsedItem(void)
{
	if (gBattleExecBuffer)
		gBattlescriptCurrInstr -= 5;
	else
	{
		gActiveBattler = 0;
		EmitDataTransfer(0, &gLastUsedItem, 2, &gLastUsedItem);
		MarkBufferBankForExecution(gActiveBattler);
	}
}

void TryToStopNewMonFromSwitchingInAfterSRHurt(void)
{
	if (gNewBS->endTurnDone)
	{
		gNewBS->restartEndTurnSwitching = TRUE;
		gBattlescriptCurrInstr = BattleScript_EntryHazardsHurtReturn - 5; //Continue the switch in effects
	}

	gNewBS->SwitchInEffectsTracker = 0;
}

extern u8 BattleScript_HandleFaintedMonDoublesSwitchInEffects[];
void ClearSwitchInEffectsTracker(void)
{
	if (!gNewBS->endTurnDone)
	{
		gBattlescriptCurrInstr = BattleScript_HandleFaintedMonDoublesSwitchInEffects - 5;
		gNewBS->SwitchInEffectsTracker = 0;
	}
};

void UpdatePrimalAbility(void)
{
	*GetAbilityLocation(gBankAttacker) = GetMonAbility(GetBankPartyData(gBankAttacker));
}

void ClearAttackerDidDamageOnce(void)
{
	gNewBS->AttackerDidDamageAtLeastOnce = 0;
}

void TryRemovePrimalWeatherOnPivot(void)
{
	if (TryRemovePrimalWeather(gBankAttacker, ABILITY(gBankAttacker)))
		gBattlescriptCurrInstr -= 5;

	if (TryActivateFlowerGift(gBankAttacker))
		gBattlescriptCurrInstr -= 5;
}

void TryRemovePrimalWeatherOnForceSwitchout(void)
{
	if (TryRemovePrimalWeather(gBankTarget, ABILITY(gBankTarget)))
		gBattlescriptCurrInstr -= 5;

	if (TryActivateFlowerGift(gBankTarget))
		gBattlescriptCurrInstr -= 5;
}

void TryRemovePrimalWeatherAfterAbilityChange(void)
{
	if (TryRemovePrimalWeather(gBankTarget, gNewBS->backupAbility))
		gBattlescriptCurrInstr -= 5;

	if (TryActivateFlowerGift(gBankTarget))
		gBattlescriptCurrInstr -= 5;
}

void TryLoadSecondFriskTargetDoubles(void)
{
	u8 partner = PARTNER(gBankTarget);

	if (IsDoubleBattle() && BATTLER_ALIVE(partner) && ITEM(partner))
	{
		gLastUsedItem = ITEM(partner);
		gBankTarget = partner;
		return;
	}

	gBattlescriptCurrInstr = BattleScript_FriskEnd - 5;
}

void RestoreAllOriginalMoveData(void)
{
	gCurrentMove = gChosenMove;

	gBankAttacker = gNewBS->originalAttackerBackup;
	gBankTarget = gNewBS->originalTargetBackup;
}

void SetBatonPassSwitchingBit(void)
{
	gNewBS->batonPassing = TRUE;
}

void ClearBatonPassSwitchingBit(void)
{
	gNewBS->batonPassing = FALSE;
}

void ReturnOpponentMon2(void)
{
	if (gBattleExecBuffer)
	{
		gBattlescriptCurrInstr -= 5;
		return;
	}

	gActiveBattler = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);

	if (IS_DOUBLE_BATTLE
	&& BATTLER_ALIVE(gActiveBattler))
	{
		EmitReturnMonToBall(0, 0);
		MarkBufferBankForExecution(gActiveBattler);
	}
}

void BackupScriptingBank(void)
{
	gNewBS->SentInBackup = gBattleScripting->bank;
}

//For Benjamin Butterfree
void RestoreEffectBankHPStatsAndRemoveBackupSpecies(void)
{
	u32 i, j;

	if (gBattleExecBuffer)
	{
		gBattlescriptCurrInstr -= 5;
		return;
	}

	//Update Moveset
	struct Pokemon* mon = GetBankPartyData(gEffectBank);
	bool8 canLearnMove[MAX_MON_MOVES] = {FALSE};
	u16 moves[MAX_LEARNABLE_MOVES + EGG_MOVES_ARRAY_COUNT + EGG_MOVES_ARRAY_COUNT + NUM_TMSHMS + (LAST_TOTAL_TUTOR_NUM + 1)] = {MOVE_NONE};
	u16 numMoves = BuildLearnableMoveset(mon, moves);

	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		u16 move = GetMonData(mon, MON_DATA_MOVE1 + i, NULL);
		if (move == MOVE_NONE)
			continue;

		for (j = 0; j < numMoves; ++j)
		{
			if (moves[j] == move)
				break;
		}

		if (j < numMoves)
			canLearnMove[i] = TRUE;
	}

	u16 newMoves[MAX_MON_MOVES] = {0};
	u16 newPP[MAX_MON_MOVES] = {0};
	u8 counter = 0;
	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		if (canLearnMove[i])
		{
			newMoves[counter] = GetMonData(mon, MON_DATA_MOVE1 + i, NULL);
			newPP[counter++] = GetMonData(mon, MON_DATA_PP1 + i, NULL);
		}
	}

	if (newMoves[0] == MOVE_NONE)
		gProtectStructs[gEffectBank].onlyStruggle = TRUE; //No moves left so struggle

	for (i = 0; i < MAX_MON_MOVES; ++i)
	{
		SetMonData(mon, MON_DATA_MOVE1 + i, &newMoves[i]); //Don't care about Emit as this isn't link compatible anyways
		SetMonData(mon, MON_DATA_PP1 + i, &newPP[i]);
		gBattleMons[gEffectBank].moves[i] = newMoves[i];
		gBattleMons[gEffectBank].pp[i] = newPP[i];
	}

	//Check if chosen move is still in moveset
	u8 originalMovePos = FindMovePositionInMoveset(gChosenMovesByBanks[gEffectBank], gEffectBank);
	if (gChosenMovesByBanks[gEffectBank] != MOVE_NONE && originalMovePos < MAX_MON_MOVES)
	{
		gBattleStruct->chosenMovePositions[gEffectBank] = originalMovePos;
		gMoveSelectionCursor[gEffectBank] = originalMovePos;
	}
	else if (counter >= 1)
	{
		gNewBS->devolveForgotMove |= gBitTable[gEffectBank]; //Can't use move anymore
		gMoveSelectionCursor[gEffectBank] = 0; //Reset selection so can't select null move
	}

	gActiveBattler = gEffectBank;
	GetBankPartyData(gActiveBattler)->backupSpecies = SPECIES_NONE; //There's no going back from a Benjamin Butterfree

	//Reset all stats
	for (i = STAT_STAGE_ATK; i < BATTLE_STATS_NO; ++i)
	{
		STAT_STAGE(gActiveBattler, i) = 6;
	}

	gBattleMons[gActiveBattler].status2 &= ~(STATUS2_CONFUSION);
	gStatuses3[gActiveBattler] &= ~(STATUS3_SWITCH_IN_ABILITY_DONE);
	gBattleMons[gActiveBattler].hp = gBattleMons[gActiveBattler].maxHP;
	EmitSetMonData(0, REQUEST_HP_BATTLE, 0, 2, &gBattleMons[gActiveBattler].hp);
	MarkBufferBankForExecution(gActiveBattler);
}

void TryActivateTargetEndTurnItemEffect(void)
{
	if (ItemBattleEffects(ItemEffects_EndTurn, gBankTarget, TRUE, FALSE))
		gBattlescriptCurrInstr -= 5;
}

void SetLaserFocusTimer(void)
{
	if (!IsLaserFocused(gBankAttacker))
		gNewBS->LaserFocusTimers[gBankAttacker] = 2;
}

void SetHealBlockTimer(void)
{
	if (!IsHealBlocked(gBankTarget))
		gNewBS->HealBlockTimers[gBankTarget] = 5;
}

void SetThroatChopTimer(void)
{
	if (!CantUseSoundMoves(gBankTarget))
		gNewBS->ThroatChopTimers[gBankTarget] = 2;
}

void SetNoMoreMovingThisTurnScriptingBank(void)
{
	gNewBS->NoMoreMovingThisTurn |= gBitTable[gBattleScripting->bank];
}