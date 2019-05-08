#pragma once

/**
 * \file attackcanceler.h
 * \brief Contains functions relating to the battle script command "attackcanceler".
 *		  This command is run at the beginning of every move and mainly deals with
 *		  things that prevent the move from executing. Also helps set up other things
 *		  like Z-Moves, multi-hit moves, and multi target moves.
 */
 
//Exported Functions
void atk00_attackcanceler(void);

//Exported Constants
enum
{
	CANCELLER_FLAGS,
	CANCELLER_ASLEEP,
	CANCELLER_FROZEN,
	CANCELLER_TRUANT,
	CANCELLER_RECHARGE,
	CANCELLER_FLINCH,
	CANCELLER_GRAVITY,
	CANCELLER_DISABLED,
	CANCELLER_HEAL_BLOCKED,
	CANCELLER_THROAT_CHOP,
	CANCELLER_TAUNTED,
	CANCELLER_IMPRISONED,
	CANCELLER_CONFUSED,
	CANCELLER_PARALYSED,
	CANCELLER_GHOST,
	CANCELLER_IN_LOVE,
	CANCELLER_STANCE_CHANGE,
	CANCELLER_BIDE,
	CANCELLER_THAW,
	CANCELLER_Z_MOVES,
	CANCELLER_GRAVITY_Z_MOVES,
	
//Called Attacks Start Here
	CANCELLER_GRAVITY_2,
	CANCELLER_SKY_BATTLE,
	CANCELLER_HEAL_BLOCKED_2,
	CANCELLER_THROAT_CHOP_2,
	CANCELLER_STANCE_CHANGE_2,
	CANCELLER_NATURAL_GIFT,
	CANCELLER_DANCER,
	CANCELLER_POWDER,
	CANCELLER_PRIMAL_WEATHER,
	CANCELLER_PSYCHIC_TERRAIN,
	CANCELLER_PRANKSTER,
	CANCELLER_EXPLODING_DAMP,
	CANCELLER_MULTIHIT_MOVES,
	CANCELLER_MULTI_TARGET_MOVES,
	CANCELLER_END,
};
