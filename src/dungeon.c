/* File: dungeon.c */

/* Purpose: the main command interpreter, updating player status */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"




/*
 * Return a "feeling" (or NULL) about an item.  Method 1 (Heavy).
 */
static cptr value_check_aux1(inven_type *i_ptr)
{
    /* Artifacts */
    if (artifact_p(i_ptr)) {

        /* Cursed/Broken */
        if (cursed_p(i_ptr) || broken_p(i_ptr)) return "terrible";

        /* Normal */
        return "special";
    }

    /* Ego-Items */
    if (ego_item_p(i_ptr)) {

        /* Cursed/Broken */
        if (cursed_p(i_ptr) || broken_p(i_ptr)) return "worthless";

        /* Normal */
        return "excellent";
    }

    /* Cursed items */
    if (cursed_p(i_ptr)) return "cursed";

    /* Broken items */
    if (broken_p(i_ptr)) return "broken";

    /* Good "armor" bonus */
    if (i_ptr->to_a > 0) return "good";

    /* Good "weapon" bonus */
    if (i_ptr->to_h + i_ptr->to_d > 0) return "good";

    /* Default to "average" */
    return "average";
}


/*
 * Return a "feeling" (or NULL) about an item.  Method 2 (Light).
 */
static cptr value_check_aux2(inven_type *i_ptr)
{
    /* Cursed items (all of them) */
    if (cursed_p(i_ptr)) return "cursed";

    /* Broken items (all of them) */
    if (broken_p(i_ptr)) return "broken";

    /* Artifacts -- except cursed/broken ones */
    if (artifact_p(i_ptr)) return "good";

    /* Ego-Items -- except cursed/broken ones */
    if (ego_item_p(i_ptr)) return "good";

    /* Good armor bonus */
    if (i_ptr->to_a > 0) return "good";

    /* Good weapon bonuses */
    if (i_ptr->to_h + i_ptr->to_d > 0) return "good";

    /* No feeling */
    return (NULL);
}




/*
 * Sense the inventory
 *
 *   Class 0 = Warrior --> fast and heavy
 *   Class 1 = Mage    --> slow and light
 *   Class 2 = Priest  --> fast but light
 *   Class 3 = Rogue   --> okay and heavy
 *   Class 4 = Ranger  --> slow and light
 *   Class 5 = Paladin --> slow but heavy
 */
static void sense_inventory(void)
{
    int		i;

    int		plev = p_ptr->lev;

    bool	heavy = FALSE;

    cptr	feel;

    inven_type *i_ptr;

    char i_name[80];


    /*** Check for "sensing" ***/

    /* Certain factors lower the "feeling" rate */
    if (p_ptr->confused || p_ptr->image) return;

    /* Analyze the class */
    switch (p_ptr->pclass) {
    
        /* Warriors */
        case 0:

            /* Good sensing */
            if (0 != rand_int(9000L / (plev * plev + 40))) return;

            /* Heavy sensing */
            heavy = TRUE;

            /* Done */
            break;
            
        /* Mages */
        case 1:

            /* Very bad (light) sensing */
            if (0 != rand_int(240000L / (plev + 5))) return;

            /* Done */
            break;

        /* Priests */
        case 2:

            /* Good (light) sensing */
            if (0 != rand_int(10000L / (plev * plev + 40))) return;

            /* Done */
            break;

        /* Rogues */
        case 3:

            /* Okay sensing */
            if (0 != rand_int(20000L / (plev * plev + 40))) return;

            /* Heavy sensing */
            heavy = TRUE;

            /* Done */
            break;
            
        /* Rangers */
        case 4:

            /* Very bad (light) sensing */
            if (0 != rand_int(120000L / (plev + 5))) return;

            /* Done */
            break;
            
        /* Paladins */
        case 5:

            /* Bad sensing */
            if (0 != rand_int(80000L / (plev * plev + 40))) return;

            /* Heavy sensing */
            heavy = TRUE;

            /* Done */
            break;
    }


    /*** Sense everything ***/

    /* Check everything */
    for (i = 0; i < INVEN_TOTAL; i++) {

        bool okay = FALSE;

        i_ptr = &inventory[i];

        /* Skip empty slots */
        if (!i_ptr->k_idx) continue;

        /* Valid "tval" codes */
        switch (i_ptr->tval) {
            case TV_SHOT:
            case TV_ARROW:
            case TV_BOLT:
            case TV_BOW:
            case TV_DIGGING:
            case TV_HAFTED:
            case TV_POLEARM:
            case TV_SWORD:
            case TV_BOOTS:
            case TV_GLOVES:
            case TV_HELM:
            case TV_CROWN:
            case TV_SHIELD:
            case TV_CLOAK:
            case TV_SOFT_ARMOR:
            case TV_HARD_ARMOR:
            case TV_DRAG_ARMOR:
                okay = TRUE;
                break;
        }

        /* Skip non-sense machines */
        if (!okay) continue;

        /* We know about it already, do not tell us again */
        if (i_ptr->ident & ID_SENSE) continue;

        /* It is fully known, no information needed */
        if (inven_known_p(i_ptr)) continue;

        /* Occasional failure on inventory items */
        if ((i < INVEN_WIELD) && (0 != rand_int(5))) continue;

        /* Check for a feeling */
        feel = (heavy ? value_check_aux1(i_ptr) : value_check_aux2(i_ptr));

        /* Skip non-feelings */
        if (!feel) continue;

        /* Stop everything */
        if (disturb_other) disturb(0, 0);

        /* Get an object description */
        objdes(i_name, i_ptr, FALSE, 0);

        /* Message (equipment) */
        if (i >= INVEN_WIELD) {
            msg_format("You feel the %s (%c) you are %s %s %s...",
                       i_name, index_to_label(i), describe_use(i),
                       ((i_ptr->number == 1) ? "is" : "are"), feel);
        }

        /* Message (inventory) */
        else {
            msg_format("You feel the %s (%c) in your pack %s %s...",
                       i_name, index_to_label(i),
                       ((i_ptr->number == 1) ? "is" : "are"), feel);
        }

        /* We have "felt" it */
        i_ptr->ident |= (ID_SENSE);

        /* Inscribe it textually */
        if (!i_ptr->note) i_ptr->note = quark_add(feel);

        /* Redraw the choice window */
        p_ptr->redraw |= (PR_CHOOSE);

        /* Combine / Reorder the pack */
        p_ptr->update |= (PU_COMBINE | PU_REORDER);
    }
}



/*
 * Regenerate hit points				-RAK-	
 */
static void regenhp(int percent)
{
    s32b        new_chp, new_chp_frac;
    int                   old_chp;

    /* Save the old hitpoints */
    old_chp = p_ptr->chp;

    /* Extract the new hitpoints */
    new_chp = ((long)p_ptr->mhp) * percent + PY_REGEN_HPBASE;
    p_ptr->chp += new_chp >> 16;   /* div 65536 */

    /* check for overflow */
    if ((p_ptr->chp < 0) && (old_chp > 0)) p_ptr->chp = MAX_SHORT;
    new_chp_frac = (new_chp & 0xFFFF) + p_ptr->chp_frac;	/* mod 65536 */
    if (new_chp_frac >= 0x10000L) {
        p_ptr->chp_frac = new_chp_frac - 0x10000L;
        p_ptr->chp++;
    }
    else {
        p_ptr->chp_frac = new_chp_frac;
    }

    /* Fully healed */
    if (p_ptr->chp >= p_ptr->mhp) {
        p_ptr->chp = p_ptr->mhp;
        p_ptr->chp_frac = 0;
    }

    /* Notice changes */
    if (old_chp != p_ptr->chp) p_ptr->redraw |= (PR_HP);
}


/*
 * Regenerate mana points				-RAK-	
 */
static void regenmana(int percent)
{
    s32b        new_mana, new_mana_frac;
    int                   old_csp;

    old_csp = p_ptr->csp;
    new_mana = ((long)p_ptr->msp) * percent + PY_REGEN_MNBASE;
    p_ptr->csp += new_mana >> 16;	/* div 65536 */
    /* check for overflow */
    if ((p_ptr->csp < 0) && (old_csp > 0)) {
        p_ptr->csp = MAX_SHORT;
    }
    new_mana_frac = (new_mana & 0xFFFF) + p_ptr->csp_frac;	/* mod 65536 */
    if (new_mana_frac >= 0x10000L) {
        p_ptr->csp_frac = new_mana_frac - 0x10000L;
        p_ptr->csp++;
    }
    else {
        p_ptr->csp_frac = new_mana_frac;
    }

    /* Must set frac to zero even if equal */
    if (p_ptr->csp >= p_ptr->msp) {
        p_ptr->csp = p_ptr->msp;
        p_ptr->csp_frac = 0;
    }

    /* Redraw mana */
    if (old_csp != p_ptr->csp) p_ptr->redraw |= (PR_MANA);
}






/*
 * Teleport player to a location (presumably near a monster)
 * This function is slightly obsessive about correctness.
 * This function allows teleporting into vaults (!)
 */
static void teleport_to(int ny, int nx)
{
    int y, x, oy, ox, dis = 0, ctr = 0;


    /* Find a usable location */
    while (1) {

        /* Pick a nearby legal location */
        while (1) {
            y = rand_spread(ny, dis);
            x = rand_spread(nx, dis);
            if (in_bounds(y, x)) break;
        }

        /* Accept "naked" floor grids */
        if (naked_grid_bold(y, x)) break;

        /* Occasionally advance the distance */
        if (++ctr > (4 * dis * dis + 4 * dis + 1)) {
            ctr = 0;
            dis++;
        }
    }

    /* Save the old location */
    oy = py;
    ox = px;
    
    /* Move the player */
    py = y;
    px = x;
    
    /* Redraw the old spot */
    lite_spot(oy, ox);
    
    /* Redraw the new spot */
    lite_spot(py, px);

    /* Check for new panel (redraw map) */
    verify_panel();
}


/*
 * Teleport the player to a new location, up to "dis" units away.
 * If no such spaces are readily available, the distance may increase.
 * Try very hard to move the player at least a quarter that distance.
 * A previous version of this function caused infinite loops.
 */
static void teleport(int dis)
{
    int d, i, min, ox, oy, x = py, y = px;

    bool look = TRUE;

    /* Minimum distance */
    min = dis / 2;

    /* Look until done */
    while (look) {

        /* Verify max distance */
        if (dis > 200) dis = 200;

        /* Try several locations */
        for (i = 0; i < 500; i++) {

            /* Pick a (possibly illegal) location */
            while (1) {
                y = rand_spread(py, dis);
                x = rand_spread(px, dis);
                d = distance(py, px, y, x);
                if ((d >= min) && (d <= dis)) break;
            }

            /* Ignore illegal locations */
            if (!in_bounds(y, x)) continue;

            /* Require "naked" floor space */
            if (!naked_grid_bold(y, x)) continue;

            /* No teleporting into vaults and such */
            if (cave[y][x].feat & CAVE_ICKY) continue;

            /* This grid looks good */
            look = FALSE;

            /* Stop looking */
            break;
        }

        /* Increase the maximum distance */
        dis = dis * 2;

        /* Decrease the minimum distance */
        min = min / 2;
    }

    /* Save the old location */
    oy = py;
    ox = px;
    
    /* Move the player */
    py = y;
    px = x;
    
    /* Redraw the old spot */
    lite_spot(oy, ox);
    
    /* Redraw the new spot */
    lite_spot(py, px);
}


/*
 * Handle teleportation
 */
static void handle_teleport(void)
{
    /* No teleport needed */
    if (!teleport_flag) return;

    /* Basic teleport */
    if (teleport_dist) teleport(teleport_dist);

    /* Directed teleport */
    else teleport_to(teleport_to_y, teleport_to_x);

    /* Forget teleport distance */
    teleport_dist = 0;
    
    /* Teleport complete */
    teleport_flag = FALSE;

    /* Check for new panel (redraw map) */
    verify_panel();

    /* Update stuff */
    p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);
    p_ptr->update |= (PU_DISTANCE);
}



/*
 * Regenerate the monsters (once per 100 game turns)
 *
 * XXX XXX XXX Should probably be done during monster turns.
 */
static void regen_monsters(void)
{
    int i, frac;

    /* Regenerate everyone */
    for (i = 1; i < m_max; i++) {

        /* Check the i'th monster */
        monster_type *m_ptr = &m_list[i];
        monster_race *r_ptr = &r_info[m_ptr->r_idx];

        /* Skip dead monsters */
        if (!m_ptr->r_idx) continue;

        /* Allow regeneration (if needed) */
        if (m_ptr->hp < m_ptr->maxhp) {

            /* Hack -- Base regeneration */
            frac = m_ptr->maxhp / 100;

            /* Hack -- Minimal regeneration rate */
            if (!frac) frac = 1;

            /* Hack -- Some monsters regenerate quickly */
            if (r_ptr->flags2 & RF2_REGENERATE) frac *= 2;

            /* Hack -- Regenerate */
            m_ptr->hp += frac;

            /* Do not over-regenerate */
            if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

            /* Redraw (later) if needed */
            if (health_who == i) p_ptr->redraw |= (PR_HEALTH);
        }
    }
}



/*
 * Handle certain things once every 10 game turns
 */
static void process_world(void)
{
    int		x, y, i, j;

    int		regen_amount;

    cave_type		*c_ptr;

    inven_type		*i_ptr;

    monster_type	*m_ptr;



    /* Every 10 game turns */
    if (turn % 10) return;


    /*** Check the Time and Load ***/

    if (!(turn % 1000)) {

        /* Check time and load */
        if ((0 != check_time()) || (0 != check_load())) {

            /* Warning */
            if (closing_flag <= 2) {

                /* Disturb */
                disturb(0, 0);

                /* Count warnings */
                closing_flag++;

                /* Message */
                msg_print("The gates to ANGBAND are closing...");
                msg_print("Please finish up and/or save your game.");
            }

            /* Slam the gate */
            else {

                /* Message */
                msg_print("The gates to ANGBAND are now closed.");

                /* Stop playing */
                alive = FALSE;
            }
        }
    }


    /*** Handle the "town" (stores and sunshine) ***/
    
    /* While in town */
    if (!dun_level) {

        /* Hack -- Daybreak/Nighfall in town */
        if (!(turn % ((10L * TOWN_DAWN) / 2))) {

            bool dawn;

            /* Check for dawn */
            dawn = (!(turn % (10L * TOWN_DAWN)));

            /* Day breaks */
            if (dawn) {

                /* Message */
                msg_print("The sun has risen.");

                /* Hack -- Scan the town */
                for (y = 0; y < cur_hgt; y++) {
                    for (x = 0; x < cur_wid; x++) {

                        /* Get the cave grid */
                        c_ptr = &cave[y][x];

                        /* Assume lit */
                        c_ptr->feat |= CAVE_GLOW;

                        /* Hack -- Memorize lit grids if allowed */
                        if (view_perma_grids) c_ptr->feat |= CAVE_MARK;

                        /* Hack -- Notice spot */
                        note_spot(y, x);
                    }
                }
            }

            /* Night falls */
            else {

                /* Message */
                msg_print("The sun has fallen.");

                /* Hack -- Scan the town */
                for (y = 0; y < cur_hgt; y++) {
                    for (x = 0; x < cur_wid; x++) {

                        /* Get the cave grid */
                        c_ptr = &cave[y][x];

                        /* Hack -- darken "boring" features */
                        if ((c_ptr->feat & 0x3F) <= 0x02) {

                            /* Forget the grid */
                            c_ptr->feat &= ~(CAVE_GLOW | CAVE_MARK);

                            /* Hack -- Notice spot */
                            note_spot(y, x);
                        }
                    }
                }
            }

            /* Update the monsters */
            p_ptr->update |= (PU_MONSTERS);

            /* Redraw stuff */
            p_ptr->redraw |= (PR_MAP);
        }
    }


    /* While in the dungeon */
    else {

        /*** Update the Stores ***/

        /* Update the stores once a day (while in dungeon) */
        if (!(turn % (10L * STORE_TURNS))) {

            /* Message */
            if (cheat_xtra) msg_print("Updating Stores...");

            /* New inventory */
            store_maint();

            /* New owners */
            if (shuffle_owners && (rand_int(STORE_SHUFFLE) == 0)) {
                if (cheat_xtra) msg_print("Shuffling a Store...");
                store_shuffle();
            }

            /* Message */
            if (cheat_xtra) msg_print("Done.");
        }
    }


    /*** Process the monsters ***/
    
    /* Check for creature generation */
    if (rand_int(MAX_M_ALLOC_CHANCE) == 0) {

        /* Make a new monster */
        (void)alloc_monster(MAX_SIGHT + 5, FALSE);
    }

    /* Hack -- Check for creature regeneration */
    if (!(turn % 100)) regen_monsters();

#ifdef SHIMMER_MONSTERS

    /* Optimize */
    if (scan_monsters) {

        /* Shimmer multi-hued monsters */
        for (i = 1; i < m_max; i++) {

            monster_race *r_ptr;

            m_ptr = &m_list[i];

            /* Skip dead monsters */
            if (!m_ptr->r_idx) continue;

            /* Skip unseen monsters */
            if (!m_ptr->ml) continue;

            /* Access the monster race */
            r_ptr = &r_info[m_ptr->r_idx];
        
            /* Skip non-multi-hued monsters */
            if (!(r_ptr->flags1 & RF1_ATTR_MULTI)) continue;
        
            /* Shimmer Multi-Hued Monsters */
            lite_spot(m_ptr->fy, m_ptr->fx);
        }

        /* Clear the flag */
        scan_monsters = FALSE;
    }

#endif


    /*** Process the objects ***/
    
    /* Recharge rods */
    for (i = 1; i < i_max; i++) {

        i_ptr = &i_list[i];

        /* Skip dead objects */
        if (!i_ptr->k_idx) continue;

        /* Recharge rods on the ground */
        if ((i_ptr->tval == TV_ROD) && (i_ptr->pval)) i_ptr->pval--;
    }

#ifdef SHIMMER_OBJECTS

#if 0
    /* Optimize */
    if (scan_objects) {

        /* Process the objects */
        for (i = 1; i < i_max; i++) {

            i_ptr = &i_list[i];
 
            /* Skip dead objects */
            if (!i_ptr->k_idx) continue;

            /* XXX XXX XXX Skip unseen objects */
        
            /* Shimmer Multi-Hued Objects XXX XXX XXX */
            lite_spot(i_ptr->iy, i_ptr->ix);
        }

        /* Clear the flag */
        scan_objects = FALSE;
    }
#endif

#endif


    /*** Damage over Time ***/

    /* Take damage from poison */
    if (p_ptr->poisoned) {

        /* Take damage */
        take_hit(1, "poison");
    }

    /* Take damage from cuts */
    if (p_ptr->cut) {

        /* Mortal wound or Deep Gash */
        if (p_ptr->cut > 200) {
            i = 3;
        }

        /* Severe cut */
        else if (p_ptr->cut > 100) {
            i = 2;
        }

        /* Other cuts */
        else {
            i = 1;
        }

        /* Take damage */
        take_hit(i, "a fatal wound");
    }


    /*** Check the Food, and Regenerate ***/

    /* Digest normally */
    if (p_ptr->food < PY_FOOD_MAX) {

        /* Every 100 game turns */
        if (!(turn % 100)) {

            /* Basic digestion rate based on speed */
            i = extract_energy[p_ptr->pspeed] * 2;

            /* Regeneration takes more food */
            if (p_ptr->regenerate) i += 30;

            /* Slow digestion takes less food */
            if (p_ptr->slow_digest) i -= 10;

            /* Digest some food */
            (void)set_food(p_ptr->food - i);
        }
    }
    
    /* Digest quickly when gorged */
    else {

        /* Digest a lot of food */
        (void)set_food(p_ptr->food - 100);
    }

    /* Starve to death (slowly) */
    if (p_ptr->food < PY_FOOD_STARVE) {

        /* Calculate damage */
        i = (PY_FOOD_STARVE - p_ptr->food) / 10;

        /* Take damage */
        take_hit(i, "starvation");
    }

    /* Default regeneration */
    regen_amount = PY_REGEN_NORMAL;

    /* Getting Weak */
    if (p_ptr->food < PY_FOOD_WEAK) {

        /* Lower regeneration */
        if (p_ptr->food < PY_FOOD_STARVE) {
            regen_amount = 0;
        }
        else if (p_ptr->food < PY_FOOD_FAINT) {
            regen_amount = PY_REGEN_FAINT;
        }
        else {
            regen_amount = PY_REGEN_WEAK;
        }

        /* Getting Faint */
        if (p_ptr->food < PY_FOOD_FAINT) {

            /* Faint occasionally */
            if (!p_ptr->paralyzed && (rand_int(100) < 10)) {

                /* Message */
                msg_print("You faint from the lack of food.");
                disturb(1, 0);

                /* Hack -- faint (bypass free action) */
                (void)set_paralyzed(p_ptr->paralyzed + 1 + rand_int(5));
            }
        }
    }

    /* Regeneration ability */
    if (p_ptr->regenerate) {
        regen_amount = regen_amount * 2;
    }

    /* Searching or Resting */
    if (p_ptr->searching || resting) {
        regen_amount = regen_amount * 2;
    }

    /* Regenerate the mana */
    if (p_ptr->csp < p_ptr->msp) {
        regenmana(regen_amount);
    }

    /* Poisoned or cut yields no healing */
    if (p_ptr->poisoned) regen_amount = 0;
    if (p_ptr->cut) regen_amount = 0;

    /* Regenerate Hit Points if needed */
    if (p_ptr->chp < p_ptr->mhp) {
        regenhp(regen_amount);
    }


    /*** Timeout Various Things ***/

    /* Hack -- Hallucinating */
    if (p_ptr->image) {
        (void)set_image(p_ptr->image - 1);
    }

    /* Blindness */
    if (p_ptr->blind) {
        (void)set_blind(p_ptr->blind - 1);
    }

    /* Times see-invisible */
    if (p_ptr->tim_invis) {
        (void)set_tim_invis(p_ptr->tim_invis - 1);
    }

    /* Timed infra-vision */
    if (p_ptr->tim_infra) {
        (void)set_tim_infra(p_ptr->tim_infra - 1);
    }

    /* Paralysis */
    if (p_ptr->paralyzed) {
        (void)set_paralyzed(p_ptr->paralyzed - 1);
    }

    /* Confusion */
    if (p_ptr->confused) {
        (void)set_confused(p_ptr->confused - 1);
    }

    /* Afraid */
    if (p_ptr->afraid) {
        (void)set_afraid(p_ptr->afraid - 1);
    }

    /* Fast */
    if (p_ptr->fast) {
        (void)set_fast(p_ptr->fast - 1);
    }

    /* Slow */
    if (p_ptr->slow) {
        (void)set_slow(p_ptr->slow - 1);
    }

    /* Protection from evil */
    if (p_ptr->protevil) {
        (void)set_protevil(p_ptr->protevil - 1);
    }

    /* Invulnerability */
    if (p_ptr->invuln) {
        (void)set_invuln(p_ptr->invuln - 1);
    }

    /* Heroism */
    if (p_ptr->hero) {
        (void)set_hero(p_ptr->hero - 1);
    }

    /* Super Heroism */
    if (p_ptr->shero) {
        (void)set_shero(p_ptr->shero - 1);
    }

    /* Blessed */
    if (p_ptr->blessed) {
        (void)set_blessed(p_ptr->blessed - 1);
    }

    /* Shield */
    if (p_ptr->shield) {
        (void)set_shield(p_ptr->shield - 1);
    }

    /* Oppose Acid */
    if (p_ptr->oppose_acid) {
        (void)set_oppose_acid(p_ptr->oppose_acid - 1);
    }

    /* Oppose Lightning */
    if (p_ptr->oppose_elec) {
        (void)set_oppose_elec(p_ptr->oppose_elec - 1);
    }

    /* Oppose Fire */
    if (p_ptr->oppose_fire) {
        (void)set_oppose_fire(p_ptr->oppose_fire - 1);
    }

    /* Oppose Cold */
    if (p_ptr->oppose_cold) {
        (void)set_oppose_cold(p_ptr->oppose_cold - 1);
    }

    /* Oppose Poison */
    if (p_ptr->oppose_pois) {
        (void)set_oppose_pois(p_ptr->oppose_pois - 1);
    }


    /*** Poison and Stun and Cut ***/

    /* Poison */
    if (p_ptr->poisoned) {

        int adjust = (adj_con_fix[p_ptr->stat_ind[A_CON]] + 1);

        /* Apply some healing */
        (void)set_poisoned(p_ptr->poisoned - adjust);
    }

    /* Stun */
    if (p_ptr->stun) {

        int adjust = (adj_con_fix[p_ptr->stat_ind[A_CON]] + 1);

        /* Apply some healing */
        (void)set_stun(p_ptr->stun - adjust);
    }

    /* Cut */
    if (p_ptr->cut) {

        int adjust = (adj_con_fix[p_ptr->stat_ind[A_CON]] + 1);

        /* Hack -- Truly "mortal" wound */
        if (p_ptr->cut > 1000) adjust = 0;

        /* Apply some healing */
        (void)set_cut(p_ptr->cut - adjust);
    }



    /*** Process Light ***/

    /* Check for light being wielded */
    i_ptr = &inventory[INVEN_LITE];

    /* Burn some fuel in the current lite */
    if (i_ptr->tval == TV_LITE) {

        /* Hack -- Use some fuel (except on artifacts) */
        if (!artifact_p(i_ptr) && (i_ptr->pval > 0)) {

            /* Decrease life-span */
            i_ptr->pval--;

            /* Hack -- notice interesting fuel steps */
            if ((i_ptr->pval < 100) || (!(i_ptr->pval % 100))) {
                /* Redraw the choice window */
                p_ptr->redraw |= (PR_CHOOSE);
            }

            /* Hack -- Special treatment when blind */
            if (p_ptr->blind) {
                /* Hack -- save some light for later */
                if (i_ptr->pval == 0) i_ptr->pval++;
            }

            /* The light is now out */
            else if (i_ptr->pval == 0) {
                disturb(0, 0);
                msg_print("Your light has gone out!");
            }

            /* The light is getting dim */
            else if ((i_ptr->pval < 100) && (!(i_ptr->pval % 10))) {
                if (disturb_other) disturb(0, 0);
                msg_print("Your light is growing faint.");
            }
        }
    }

    /* Extract the current lite radius */
    extract_cur_lite();

    /* Extract the current view radius */
    extract_cur_view();


    /*** Process Inventory ***/

    /* Handle experience draining */
    if (p_ptr->exp_drain) {
        if ((rand_int(100) < 10) && (p_ptr->exp > 0)) {
            p_ptr->exp--;
            p_ptr->max_exp--;
            check_experience();
        }
    }

    /* Note changes */
    j = 0;

    /* Process equipment */
    for (i = INVEN_WIELD; i < INVEN_TOTAL; i++) {

        /* Get the object */
        i_ptr = &inventory[i];

        /* Skip non-objects */
        if (!i_ptr->k_idx) continue;

        /* Recharge activatable objects */
        if (i_ptr->timeout > 0) {

            /* Recharge */
            i_ptr->timeout--;

            /* Notice changes */
            /* if (!(i_ptr->timeout)) j++; */
        }
    }

    /* Recharge rods */
    for (i = 0; i < INVEN_PACK; i++) {

        i_ptr = &inventory[i];

        /* Examine all charging rods */
        if ((i_ptr->tval == TV_ROD) && (i_ptr->pval)) {

            /* Charge it */
            i_ptr->pval--;

            /* Notice changes */
            if (!(i_ptr->pval)) j++;
        }
    }

    /* Notice changes */
    if (j) {

        /* Redraw the Choice Window */
        p_ptr->redraw |= (PR_CHOOSE);

        /* Combine pack */
        p_ptr->update |= (PU_COMBINE);
    }

    /* Feel the inventory */
    sense_inventory();


    /*** Involuntary Movement ***/

    /* Delayed Word-of-Recall */
    if (p_ptr->word_recall) {

        /* Count down towards recall */
        p_ptr->word_recall--;

        /* Activate the recall */
        if (!p_ptr->word_recall) {

            /* Disturbing! */
            disturb(0,0);

            /* Determine the level */
            if (dun_level) {
                msg_print("You feel yourself yanked upwards!");
                dun_level = 0;
                new_level_flag = TRUE;
            }
            else {
                msg_print("You feel yourself yanked downwards!");
                dun_level = p_ptr->max_dlv;
                if (!dun_level) dun_level = 1;
                new_level_flag = TRUE;
            }
        }
    }
}



/*
 * Process the player
 */
static void process_player()
{
    int			i;

    inven_type		*i_ptr;


    /* Give the player some energy */
    p_ptr->energy += extract_energy[p_ptr->pspeed];

    /* No turn yet */
    if (p_ptr->energy < 100) return;


    /*** Handle Resting ***/

    /* Check "Resting" status */
    if (resting) {

        /* +n -> rest for n turns */
        if (resting > 0) {

            /* Reduce rest count */
            resting--;

            /* Redraw the state */
            p_ptr->redraw |= (PR_STATE);
        }

        /* -1 -> rest until HP/mana restored */
        else if (resting == -1) {

            /* Stop resting */
            if ((p_ptr->chp == p_ptr->mhp) &&
                (p_ptr->csp == p_ptr->msp)) {

                disturb(0, 0);
            }
        }

        /* -2 -> like -1, plus blind/conf/fear/stun/slow/stone/halluc/recall */
        /* Note: stop (via "disturb") as soon as blind or recall is done */
        else if (resting == -2) {

            /* Stop resting */
            if ((p_ptr->chp == p_ptr->mhp) &&
                (p_ptr->csp == p_ptr->msp) &&
                !p_ptr->blind && !p_ptr->confused &&
                !p_ptr->poisoned && !p_ptr->afraid &&
                !p_ptr->stun && !p_ptr->cut &&
                !p_ptr->slow && !p_ptr->paralyzed &&
                !p_ptr->image && !p_ptr->word_recall) {

                disturb(0, 0);
            }
        }
    }



    /*** Handle actual user input ***/

    /* Hack -- Check for "player interrupts" */
    if (command_rep || running || resting) {

        /* Do not wait */
        inkey_scan = TRUE;

        /* Check for a key */
        if (inkey()) {

            /* Flush input */
            flush();

            /* Hack -- Show a Message */
            msg_print("Cancelled.");

            /* Disturb the resting, running, or repeating */
            disturb(0, 0);

            /* Hack -- Redraw the state */
            p_ptr->redraw |= (PR_STATE);
        }
    }


    /* Mega-Hack -- Random teleportation */
    if ((p_ptr->teleport) && (rand_int(100) < 1)) {

        /* Short range teleport */
        teleport_flag = TRUE;
        teleport_dist = 40;
    }


    /* Repeat until out of energy */
    while (p_ptr->energy >= 100) {


        /* Hack -- Notice death or departure */
        if (!alive || new_level_flag) break;


        /* Hack -- Process Teleportation */
        if (teleport_flag) handle_teleport();

        /* Hack -- constant hallucination */
        if (p_ptr->image) p_ptr->redraw |= (PR_MAP);


        /* Handle stuff */
        handle_stuff();

        /* Hilite the player */
        move_cursor_relative(py, px);

        /* Refresh (optional) */
        if (fresh_before) Term_fresh();


        /* Hack -- cancel "lurking browse mode" */
        if (!command_new) command_see = FALSE;


        /* Assume free turn */
        energy_use = 0;
        

        /* Hack -- Resting */
        if ((resting) ||
            (p_ptr->paralyzed) ||
            (p_ptr->stun >= 100)) {

            /* Take a turn */
            energy_use = 100;

            /* Refresh */
            Term_fresh();
        }

        /* Hack -- Running */
        else if (running) {

            /* Take a turn */
            energy_use = 100;

            /* Take a step */
            do_cmd_run();
        }

        /* Normal command */
        else {

            /* Get a command (new or old) */
            request_command();

            /* Process the command */
            process_command();
        }


        /* Hack -- handle pack over-flow */
        if (inventory[INVEN_PACK].k_idx) {

            int		amt;

            char	i_name[80];


            /* Choose an item to spill */
            i = INVEN_PACK;

            /* Access the slot to be dropped */
            i_ptr = &inventory[i];

            /* Drop all of that item */
            amt = i_ptr->number;

            /* Disturbing */
            disturb(0, 0);

            /* Warning */
            msg_print("Your pack overflows!");

            /* Describe */
            objdes(i_name, i_ptr, TRUE, 3);

            /* Message */
            msg_format("You drop %s.", i_name);

            /* Drop it (carefully) near the player */
            drop_near(i_ptr, 0, py, px);

            /* Decrease the item, optimize. */
            inven_item_increase(i, -amt);
            inven_item_optimize(i);
        }


        /* Use some energy, if required */
        if (energy_use) p_ptr->energy -= energy_use;
    }


    /* Hack -- notice death or departure */
    if (!alive || new_level_flag) return;


    /* Hack -- Process Teleportation */
    if (teleport_flag) handle_teleport();


    /* Handle stuff (one last time) */
    handle_stuff();
}



/*
 * Interact with the current dungeon level.
 *
 * This function will not exit until the level is completed,
 * the user dies, or the game is terminated.
 */
static void dungeon(void)
{
    /* Reset various flags */
    new_level_flag = FALSE;
    teleport_flag = FALSE;

    /* Not running */
    running = 0;

    /* Not resting */
    resting = 0;

    /* Reset the "command" vars */
    command_cmd = 0;
    command_new = 0;
    command_rep = 0;
    command_arg = 0;
    command_dir = 0;


    /* Cancel the target */
    target_who = 0;

    /* Cancel the health bar */
    health_track(0);


    /* Turn off searching */
    search_off();


    /* Remember deepest dungeon level visited */
    if (dun_level > (unsigned)(p_ptr->max_dlv)) {
        p_ptr->max_dlv = dun_level;
    }


    /* Paranoia -- No stairs down from Quest */
    if (is_quest(dun_level)) create_down_stair = FALSE;

    /* Paranoia -- no stairs from town */
    if (!dun_level) create_down_stair = create_up_stair = FALSE;

    /* Option -- no connected stairs */
    if (!dungeon_stair) create_down_stair = create_up_stair = FALSE;

    /* Make a stairway. */
    if (create_up_stair || create_down_stair) {

        /* Place a stairway */
        if (valid_grid(py, px)) {

            cave_type *c_ptr;

            /* Delete the old object */
            delete_object(py, px);

            /* Access the cave grid */
            c_ptr = &cave[py][px];

            /* Make stairs */
            if (create_down_stair) {
                c_ptr->feat = ((c_ptr->feat & ~0x3F) | 0x07);
            }
            else {
                c_ptr->feat = ((c_ptr->feat & ~0x3F) | 0x06);
            }
        }

        /* Cancel the stair request */
        create_down_stair = create_up_stair = FALSE;
    }


    /* Choose a panel row */
    panel_row = ((py - SCREEN_HGT / 4) / (SCREEN_HGT / 2));
    if (panel_row > max_panel_rows) panel_row = max_panel_rows;
    else if (panel_row < 0) panel_row = 0;

    /* Choose a panel col */
    panel_col = ((px - SCREEN_WID / 4) / (SCREEN_WID / 2));
    if (panel_col > max_panel_cols) panel_col = max_panel_cols;
    else if (panel_col < 0) panel_col = 0;

    /* Recalculate the boundaries */
    panel_bounds();


    /* Flush messages */
    msg_print(NULL);
    
    
    /* Hack -- Verify the lite radius */
    extract_cur_lite();

    /* Hack -- Verify the view radius */
    extract_cur_view();

    /* Redraw the recall window */
    p_ptr->redraw |= (PR_RECENT);

    /* Redraw the choice window */
    p_ptr->redraw |= (PR_CHOOSE);

    /* Redraw dungeon */
    p_ptr->redraw |= (PR_WIPE | PR_MAP | PR_BASIC | PR_EXTRA);

    /* Update stuff */
    p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_DISTANCE);

    /* Handle stuff */
    handle_stuff();


    /* Update stuff */
    p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);

    /* Combine / Reorder the pack */
    p_ptr->update |= (PU_COMBINE | PU_REORDER);

    /* Handle stuff */
    handle_stuff();

    /* Refresh */
    Term_fresh();


    /* Announce (or repeat) the feeling */
    if (dun_level) do_cmd_feeling();


    /*** Process this dungeon level ***/

    /* Reset the monster generation level */
    monster_level = dun_level;

    /* Reset the object generation level */
    object_level = dun_level;

    /* Main loop */
    while (TRUE) {

        /* Hack -- Compact the object list occasionally */
        if (i_cnt + 16 > MAX_I_IDX) compact_objects(32);

        /* Hack -- Compact the monster list occasionally */
        if (m_cnt + 32 > MAX_M_IDX) compact_monsters(64);

        /* Process the player */
        process_player();

        /* Hack -- Notice death or departure */
        if (!alive || new_level_flag) break;

        /* Process all of the monsters */
        process_monsters();

        /* Hack -- Notice death or departure */
        if (!alive || new_level_flag) break;

        /* Process the world */
        process_world();

        /* Hack -- Notice death or departure */
        if (!alive || new_level_flag) break;

        /* Count game turns */
        turn++;
    }



    /* Handle stuff */
    handle_stuff();


    /* Cancel the target */
    target_who = 0;

    /* Cancel the health bar */
    health_track(0);


    /* Forget the old lite */
    forget_lite();

    /* Forget the old view */
    forget_view();
}




/*
 * Load the various "user pref files"
 */
static void load_all_pref_files(void)
{
    char buf[1024];


    /* Access the "basic" pref file */
    strcpy(buf, "pref.prf");

    /* Process that file */
    process_pref_file(buf);

    /* Access the "user" pref file */
    sprintf(buf, "user.prf");

    /* Process that file */
    process_pref_file(buf);



    /* Access the "basic" system pref file */
    sprintf(buf, "pref-%s.prf", ANGBAND_SYS);

    /* Process that file */
    process_pref_file(buf);

    /* Access the "visual" system pref file (if any) */
    sprintf(buf, "%s-%s.prf", (use_graphics ? "graf" : "font"), ANGBAND_SYS);

    /* Process that file */
    process_pref_file(buf);

    /* Access the "user" system pref file */
    sprintf(buf, "user-%s.prf", ANGBAND_SYS);

    /* Process that file */
    process_pref_file(buf);


    /* Access the "race" pref file */
    sprintf(buf, "%s.prf", rp_ptr->title);

    /* Process that file */
    process_pref_file(buf);

    /* Access the "class" pref file */
    sprintf(buf, "%s.prf", cp_ptr->title);

    /* Process that file */
    process_pref_file(buf);

    /* Access the "character" pref file */
    sprintf(buf, "%s.prf", player_base);

    /* Process that file */
    process_pref_file(buf);
}


/*
 * Actually play a game
 */
void play_game(bool new_game)
{
    u32b seed;


    /* Hack -- Character is "icky" */
    character_icky = TRUE;


    /* Hack -- turn off the cursor */
    Term_hide_cursor();


    /* Basic seed */
    seed = (time(NULL));
    
#ifdef SET_UID

    /* Mutate the seed on Unix machines */
    seed = ((seed >> 3) * (getpid() << 1));

#endif

    /* Use the complex RNG */
    Rand_quick = FALSE;

    /* Seed the "complex" RNG */
    Rand_state_init(seed);


    /* Load old character */
    if (!new_game) {

        /* Attempt to load, note dead player */
        if (!load_player()) new_game = TRUE;

        /* Process the player name */
        process_player_name(FALSE);
    }


    /* Roll new character */
    if (new_game) {

        /* Hack -- seed for flavors */
        seed_flavor = rand_int(0x10000000);

        /* Hack -- seed for town layout */
        seed_town = rand_int(0x10000000);

        /* Roll up a new character */
        player_birth();

        /* Hack -- enter the world */
        turn = 1;
    }


    /* Flash a message */
    prt("Please wait...", 0, 0);

    /* Flush the message */
    Term_fresh();


    /* Enter wizard mode AFTER "resurrection" */
    if (arg_wizard && enter_wiz_mode()) wizard = TRUE;


    /* Mega-Hack -- enforce "delayed death" */
    if (p_ptr->chp < 0) death = TRUE;


    /* Flavor the objects */
    flavor_init();

    /* Reset the visual mappings */
    reset_visuals();


    /* Redraw the recall window */
    p_ptr->redraw |= (PR_RECENT);

    /* Redraw the choice window */
    p_ptr->redraw |= (PR_CHOOSE);

    /* Handle stuff */
    handle_stuff();


    /* Set or clear "rogue_like_commands" if requested */
    if (arg_force_original) rogue_like_commands = FALSE;
    if (arg_force_roguelike) rogue_like_commands = TRUE;

    /* Verify the keymap */
    keymap_init();

    /* Load the "pref" files */
    load_all_pref_files();

    /* Verify the keymap again */
    keymap_init();

    /* React to changes */
    Term_xtra(TERM_XTRA_REACT, 0);


    /* Make a level if necessary */
    if (!character_dungeon) generate_cave();


    /* Character is now "complete" */
    character_generated = TRUE;


    /* Hack -- Character is no longer "icky" */
    character_icky = FALSE;

    
    /* Start game */
    alive = !death;

    /* Loop till dead */
    while (TRUE) {

        /* Process the level */
        dungeon();

        /* Handle "quit" without "death" */
        if (!alive && !death) break;

        /* Erase the old cave */
        wipe_i_list();
        wipe_m_list();

        /* Handle "quit" from "death" */
        if (!alive) break;

        /* Make a new level */
        generate_cave();
    }

    /* Close stuff */
    close_game();

    /* Quit */
    quit(NULL);
}


