// mod-temucraft-classes: script loader
void AddTemucraftTalentsScripts();
void AddTemucraftReagentsScripts();
void AddTemucraftRequirementsScripts();
void AddTemucraftSpellInfoScripts();

void Addmod_temucraft_class_tweaksScripts();
void Addmod_temucraft_new_spellsScripts();
void Addmod_temucraft_racialsScripts();
void Addmod_temucraft_pushbackScripts();
void Addmod_temucraft_ranged_autoattackScripts();
void Addmod_temucraft_classesScripts()
{
    AddTemucraftTalentsScripts();
    AddTemucraftReagentsScripts();
    AddTemucraftRequirementsScripts();
    AddTemucraftSpellInfoScripts();   // spell tooltip details (TSI)
    Addmod_temucraft_class_tweaksScripts();   // was mod-temucraft-class-tweaks
    Addmod_temucraft_new_spellsScripts();   // was mod-temucraft-new-spells
    Addmod_temucraft_racialsScripts();   // was mod-temucraft-racials
    Addmod_temucraft_pushbackScripts();   // was mod-temucraft-pushback
    Addmod_temucraft_ranged_autoattackScripts();   // was mod-temucraft-ranged-autoattack
}
