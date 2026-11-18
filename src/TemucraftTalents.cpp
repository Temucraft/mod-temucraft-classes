/*
 * mod-temucraft-classes: scripts for the custom talents that spell data can't do alone.
 *
 *   Bloom (95145)               Rejuvenation or Regrowth running out heals the target for 100% of the druid's Spirit
 *                               (50% when dispelled): spell_temucraft_bloom on every rank of both (spell_script_names).
 *   Templar's Bulwark (95147)   the absorb shield is the paladin's maximum health.
 *   Lone Wolf (95150)           +20% damage (95151) while the hunter has no pet out.
 *   Focused Fire (95152/95153)  +1%/+2% damage (95154) while the pet is out.
 * Batch 4: Predatory Discipline, Spearing Strike, Arcane Wrath, Light's Vigil, Unsealed Wrath, Bane of Havoc,
 *   Demonic Energies, Demonic Brand, Lingering Rage (below).
 * The rest (Heating Up, Touch of the Grave, Reckless Swings, ...) is spell data plus spell_proc rows in data/sql.
 */

#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "Pet.h"
#include "GameTime.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "SpellMgr.h"
#include "UnitScript.h"

#include <unordered_map>

namespace
{
    constexpr uint32 SPELL_BLOOM_TALENT = 95145;
    constexpr uint32 SPELL_BLOOM_HEAL = 95146;
}

class spell_temucraft_bloom : public AuraScript
{
    PrepareAuraScript(spell_temucraft_bloom);

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        AuraRemoveMode removeMode = GetTargetApplication()->GetRemoveMode();
        if (removeMode != AURA_REMOVE_BY_EXPIRE && removeMode != AURA_REMOVE_BY_ENEMY_SPELL)
            return;
        Unit* caster = GetCaster();
        Unit* target = GetTarget();
        if (!caster || !target || !target->IsAlive() || !caster->HasAura(SPELL_BLOOM_TALENT))
            return;
        int32 heal = int32(caster->GetStat(STAT_SPIRIT) * (removeMode == AURA_REMOVE_BY_EXPIRE ? 1.0f : 0.5f));
        if (heal > 0)
            caster->CastCustomSpell(target, SPELL_BLOOM_HEAL, &heal, nullptr, nullptr, true);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_temucraft_bloom::HandleRemove, EFFECT_ALL, SPELL_AURA_PERIODIC_HEAL,
            AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_temucraft_templars_bulwark : public AuraScript
{
    PrepareAuraScript(spell_temucraft_templars_bulwark);

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
    {
        canBeRecalculated = false;
        if (Unit* caster = GetCaster())
            amount = int32(caster->GetMaxHealth());
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_temucraft_templars_bulwark::CalculateAmount, EFFECT_0,
            SPELL_AURA_SCHOOL_ABSORB);
    }
};

// Lone Wolf and Focused Fire follow the pet, checked every second
class TemucraftTalents_Player : public PlayerScript
{
public:
    TemucraftTalents_Player() : PlayerScript("TemucraftTalents_Player", { PLAYERHOOK_ON_UPDATE }) { }

    void OnPlayerUpdate(Player* player, uint32 diff) override
    {
        if (player->getClass() != CLASS_HUNTER)
            return;
        uint32& timer = _timers[player->GetGUID().GetCounter()];
        if (timer > diff)
        {
            timer -= diff;
            return;
        }
        timer = 1000;
        bool pet = player->GetPet() != nullptr;
        Toggle(player, 95151, player->HasSpell(95150) && !pet, 0);
        int32 focused = player->HasSpell(95153) ? 2 : player->HasSpell(95152) ? 1 : 0;
        Toggle(player, 95154, focused && pet, focused);
    }

private:
    std::unordered_map<ObjectGuid::LowType, uint32> _timers;

    static void Toggle(Player* player, uint32 spell, bool on, int32 amount)
    {
        Aura* aura = player->GetAura(spell);
        if (!on)
        {
            if (aura)
                player->RemoveAura(spell);
            return;
        }
        if (!aura)
            aura = player->AddAura(spell, player);
        if (aura && amount)
            if (AuraEffect* effect = aura->GetEffect(EFFECT_0))
                if (effect->GetAmount() != amount)
                    effect->ChangeAmount(amount);
    }
};

// ---- batch 4: talents that are mostly script ----------------------------------------------------
namespace
{
    constexpr uint32 SPELL_PREDATORY_DISCIPLINE = 95240;
    constexpr uint32 SPELLS_LINGERING_RAGE[] = { 95241, 95242, 95243, 95244, 95245 };
    constexpr uint32 SPELLS_DEMONIC_ENERGIES[] = { 95246, 95247 };
    constexpr uint32 SPELL_DEMONIC_ENERGIES_HEAL = 95270;
    constexpr uint32 SPELLS_DEMONIC_BRAND[] = { 95263, 95265, 95267 }; // +10/20/30%
    constexpr uint32 SPELL_BANE_OF_HAVOC = 95268;
    constexpr uint32 SPELL_BANE_OF_HAVOC_DAMAGE = 95269;
    constexpr uint32 SPELL_LIGHTS_VIGIL = 95237;
    constexpr uint32 SPELL_LIGHTS_VIGIL_HEAL = 95271;
    constexpr uint32 SPELL_JUDGEMENT_DAMAGE = 54158;

    int32 TalentRank(Unit const* unit, uint32 const* spells, size_t count)
    {
        for (size_t i = count; i > 0; --i)
            if (unit->HasAura(spells[i - 1]))
                return int32(i);
        return 0;
    }

    std::unordered_map<ObjectGuid, ObjectGuid> HavocTargets; // warlock -> cursed target
}

// Predatory Discipline: Ferocious Bite gives back the extra Energy it took (the damage was already counted)
class spell_temucraft_predatory_discipline : public SpellScript
{
    PrepareSpellScript(spell_temucraft_predatory_discipline);

    void Record(SpellEffIndex /*effIndex*/)
    {
        if (GetCaster()->HasAura(SPELL_PREDATORY_DISCIPLINE))
            _energy = GetCaster()->GetPower(POWER_ENERGY);
    }

    void Refund()
    {
        if (_energy < 0)
            return;
        int32 used = _energy - int32(GetCaster()->GetPower(POWER_ENERGY));
        if (used > 0)
            GetCaster()->ModifyPower(POWER_ENERGY, used);
        _energy = -1;
    }

    void Register() override
    {
        OnEffectLaunchTarget += SpellEffectFn(spell_temucraft_predatory_discipline::Record, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        AfterHit += SpellHitFn(spell_temucraft_predatory_discipline::Refund);
    }

    int32 _energy = -1;
};

// Spearing Strike: double damage to giants, dragonkin and mounted targets, and knocks riders off
class spell_temucraft_spearing_strike : public SpellScript
{
    PrepareSpellScript(spell_temucraft_spearing_strike);

    void HandleHit()
    {
        Unit* target = GetHitUnit();
        if (!target)
            return;
        bool bonus = target->IsMounted();
        if (Creature* creature = target->ToCreature())
            bonus = bonus || creature->GetCreatureType() == CREATURE_TYPE_GIANT || creature->GetCreatureType() == CREATURE_TYPE_DRAGONKIN;
        if (bonus)
            SetHitDamage(GetHitDamage() * 2);
        if (target->IsMounted())
        {
            target->RemoveAurasByType(SPELL_AURA_MOUNTED);
            target->Dismount();
        }
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_temucraft_spearing_strike::HandleHit);
    }
};

// Arcane Wrath: +5% of maximum mana as damage
class spell_temucraft_arcane_wrath : public SpellScript
{
    PrepareSpellScript(spell_temucraft_arcane_wrath);

    void HandleHit()
    {
        SetHitDamage(GetHitDamage() + int32(GetCaster()->GetMaxPower(POWER_MANA) * 0.05f));
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_temucraft_arcane_wrath::HandleHit);
    }
};

// Light's Vigil: Holy Shock on a target with your vigil has no cooldown and heals the target's party
class spell_temucraft_lights_vigil : public SpellScript
{
    PrepareSpellScript(spell_temucraft_lights_vigil);

    void HandleHit()
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!target || !target->HasAura(SPELL_LIGHTS_VIGIL, caster->GetGUID()))
            return;
        target->RemoveAurasDueToSpell(SPELL_LIGHTS_VIGIL, caster->GetGUID());
        _reset = true;
        Player* targetPlayer = target->ToPlayer();
        Group* group = targetPlayer ? targetPlayer->GetGroup() : nullptr;
        if (!group)
            return;
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (member && member != target && member->IsAlive() && member->IsInMap(target) &&
                group->SameSubGroup(targetPlayer, member) && member->GetDistance(target) <= 30.0f)
                caster->CastSpell(member, SPELL_LIGHTS_VIGIL_HEAL, true);
        }
    }

    void HandleAfterCast()
    {
        if (_reset)
            if (Player* player = GetCaster()->ToPlayer())
                player->RemoveSpellCooldown(GetSpellInfo()->Id, true);
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_temucraft_lights_vigil::HandleHit);
        AfterCast += SpellCastFn(spell_temucraft_lights_vigil::HandleAfterCast);
    }

    bool _reset = false;
};

// Unsealed Wrath: unleash the active seal's Judgement damage without the Judgement itself
class spell_temucraft_unsealed_wrath : public SpellScript
{
    PrepareSpellScript(spell_temucraft_unsealed_wrath);

    void HandleScript(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!target)
            return;
        bool sealed = false;
        uint32 damageSpell = SPELL_JUDGEMENT_DAMAGE;
        for (AuraEffect const* effect : caster->GetAuraEffectsByType(SPELL_AURA_DUMMY))
        {
            if (effect->GetSpellInfo()->GetSpellSpecific() != SPELL_SPECIFIC_SEAL)
                continue;
            sealed = true;
            if (effect->GetEffIndex() == EFFECT_2 && sSpellMgr->GetSpellInfo(effect->GetAmount()))
            {
                damageSpell = effect->GetAmount();
                break;
            }
        }
        if (sealed)
            caster->CastSpell(target, damageSpell, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_temucraft_unsealed_wrath::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// Bane of Havoc: one cursed target per warlock
class spell_temucraft_bane_of_havoc : public AuraScript
{
    PrepareAuraScript(spell_temucraft_bane_of_havoc);

    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;
        ObjectGuid& current = HavocTargets[caster->GetGUID()];
        if (current && current != GetTarget()->GetGUID())
            if (Unit* old = ObjectAccessor::GetUnit(*caster, current))
                old->RemoveAurasDueToSpell(SPELL_BANE_OF_HAVOC, caster->GetGUID());
        current = GetTarget()->GetGUID();
    }

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        auto itr = HavocTargets.find(GetCasterGUID());
        if (itr != HavocTargets.end() && itr->second == GetTarget()->GetGUID())
            HavocTargets.erase(itr);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_temucraft_bane_of_havoc::HandleApply, EFFECT_0, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_temucraft_bane_of_havoc::HandleRemove, EFFECT_0, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
    }
};

// Demonic Energies: Life Tap also restores 10% (rank 2: 20%) of the pet's mana
class spell_temucraft_demonic_energies_tap : public SpellScript
{
    PrepareSpellScript(spell_temucraft_demonic_energies_tap);

    void HandleAfterCast()
    {
        Player* player = GetCaster()->ToPlayer();
        if (!player)
            return;
        int32 rank = TalentRank(player, SPELLS_DEMONIC_ENERGIES, 2);
        Pet* pet = player->GetPet();
        if (rank && pet && pet->IsAlive() && pet->GetMaxPower(POWER_MANA))
            pet->ModifyPower(POWER_MANA, int32(pet->GetMaxPower(POWER_MANA) * 0.10f * rank));
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_temucraft_demonic_energies_tap::HandleAfterCast);
    }
};

// Bane of Havoc splash, Demonic Energies pet heal, Demonic Brand pet bonus
class TemucraftTalents_Unit : public UnitScript
{
public:
    TemucraftTalents_Unit() : UnitScript("TemucraftTalents_Unit", true,
        { UNITHOOK_ON_DAMAGE, UNITHOOK_MODIFY_MELEE_DAMAGE, UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN }) { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (_busy || !attacker || !victim || attacker == victim || !damage)
            return;
        Player* player = attacker->ToPlayer();
        if (!player || player->getClass() != CLASS_WARLOCK)
            return;
        _busy = true;
        auto itr = HavocTargets.find(player->GetGUID());
        if (itr != HavocTargets.end() && itr->second != victim->GetGUID())
        {
            Unit* cursed = ObjectAccessor::GetUnit(*player, itr->second);
            if (cursed && cursed->IsAlive() && cursed->IsInMap(player) && cursed->GetDistance(player) <= 60.0f)
            {
                int32 splash = int32(damage * 0.15f);
                if (splash > 0)
                    player->CastCustomSpell(cursed, SPELL_BANE_OF_HAVOC_DAMAGE, &splash, nullptr, nullptr, true);
            }
        }
        if (int32 rank = TalentRank(player, SPELLS_DEMONIC_ENERGIES, 2))
        {
            Pet* pet = player->GetPet();
            if (pet && pet != victim && pet->IsAlive() && !pet->IsFullHealth())
            {
                int32 heal = int32(damage * 0.04f * rank);
                if (heal > 0)
                    player->CastCustomSpell(pet, SPELL_DEMONIC_ENERGIES_HEAL, &heal, nullptr, nullptr, true);
            }
        }
        _busy = false;
    }

    void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
    {
        if (int32 pct = Brand(target, attacker))
            damage += damage * pct / 100;
    }

    void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* /*spellInfo*/) override
    {
        if (int32 pct = Brand(target, attacker))
            damage += damage * pct / 100;
    }

private:
    bool _busy = false;

    static int32 Brand(Unit* target, Unit* attacker)
    {
        if (!target || !attacker || !attacker->IsPet())
            return 0;
        Unit* owner = attacker->GetOwner();
        if (!owner)
            return 0;
        for (size_t i = 3; i > 0; --i)
            if (Aura* aura = target->GetAura(SPELLS_DEMONIC_BRAND[i - 1], owner->GetGUID()))
            {
                aura->DropCharge();
                return int32(i) * 10;
            }
        return 0;
    }
};

// Lingering Rage: out of combat, rage decay waits 2 sec per rank
class TemucraftTalents_Rage : public PlayerScript
{
public:
    TemucraftTalents_Rage() : PlayerScript("TemucraftTalents_Rage", { PLAYERHOOK_ON_UPDATE }) { }

    void OnPlayerUpdate(Player* player, uint32 /*diff*/) override
    {
        if (player->getClass() != CLASS_WARRIOR && player->getClass() != CLASS_DRUID)
            return;
        State& state = _states[player->GetGUID().GetCounter()];
        uint32 rage = player->GetPower(POWER_RAGE);
        uint32 now = GameTime::GetGameTimeMS().count();
        if (player->IsInCombat())
        {
            state.leftCombat = now;
            state.rage = rage;
            return;
        }
        int32 rank = TalentRank(player, SPELLS_LINGERING_RAGE, 5);
        // only undo the natural decay (2 rage a tick), never rage spent on abilities
        if (rank && now - state.leftCombat < uint32(rank) * 2000 && rage < state.rage && state.rage - rage <= 20)
            player->SetPower(POWER_RAGE, state.rage);
        else
            state.rage = rage;
    }

private:
    struct State
    {
        uint32 leftCombat = 0;
        uint32 rage = 0;
    };
    std::unordered_map<ObjectGuid::LowType, State> _states;
};

void AddTemucraftTalentsScripts()
{
    new TemucraftTalents_Player();
    RegisterSpellScript(spell_temucraft_bloom);
    RegisterSpellScript(spell_temucraft_templars_bulwark);
    new TemucraftTalents_Unit();
    new TemucraftTalents_Rage();
    RegisterSpellScript(spell_temucraft_predatory_discipline);
    RegisterSpellScript(spell_temucraft_spearing_strike);
    RegisterSpellScript(spell_temucraft_arcane_wrath);
    RegisterSpellScript(spell_temucraft_lights_vigil);
    RegisterSpellScript(spell_temucraft_unsealed_wrath);
    RegisterSpellScript(spell_temucraft_bane_of_havoc);
    RegisterSpellScript(spell_temucraft_demonic_energies_tap);
}
