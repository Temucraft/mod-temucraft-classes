-- Swift Judgement (95259, Forever paladin talent): its only effect is a script effect, like the stock Judgements,
-- but no script was bound, so the cast did nothing. Bind the stock Judgement of Light script: it judges the
-- active seal (damage, Judgements of the Just, Seal of Command cleave, Tier 5 mana) and applies Judgement of Light.
-- Its 1 min cooldown is its own (Spell.dbc RecoveryTime, category 0), so it doesn't share the Judgements' cooldown.
DELETE FROM `spell_script_names` WHERE `spell_id` = 95259;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (95259, 'spell_pal_judgement_of_light');
