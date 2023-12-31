// DO NOT EDIT THIS FILE
// This file was automatically generated by configuration-parser
// There will be a .conf file somewhere which was used to generate this file
// See https://github.com/tcanabrava/configuration-parser

#include "test-simple-prefs.h"
#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>

Status::Status(QObject *parent) : QObject(parent),
	_strength(10),
	_wisdom(10),
	_agility(10),
	_inteligence(10)
{
}

int Status::strength() const
{
	return _strength;
}

int Status::wisdom() const
{
	return _wisdom;
}

int Status::agility() const
{
	return _agility;
}

int Status::inteligence() const
{
	return _inteligence;
}

void Status::setStrength(int value)
{
	if (_strength==value) {
		return;
	}
	if (strengthRule && !strengthRule(value)) {
		return;
	}
	_strength = value;
	Q_EMIT strengthChanged(value);
}

void Status::setWisdom(int value)
{
	if (_wisdom==value) {
		return;
	}
	if (wisdomRule && !wisdomRule(value)) {
		return;
	}
	_wisdom = value;
	Q_EMIT wisdomChanged(value);
}

void Status::setAgility(int value)
{
	if (_agility==value) {
		return;
	}
	if (agilityRule && !agilityRule(value)) {
		return;
	}
	_agility = value;
	Q_EMIT agilityChanged(value);
}

void Status::setInteligence(int value)
{
	if (_inteligence==value) {
		return;
	}
	if (inteligenceRule && !inteligenceRule(value)) {
		return;
	}
	_inteligence = value;
	Q_EMIT inteligenceChanged(value);
}

void Status::setStrengthRule(std::function<bool(int)> rule)
{
	strengthRule = rule;
}

void Status::setWisdomRule(std::function<bool(int)> rule)
{
	wisdomRule = rule;
}

void Status::setAgilityRule(std::function<bool(int)> rule)
{
	agilityRule = rule;
}

void Status::setInteligenceRule(std::function<bool(int)> rule)
{
	inteligenceRule = rule;
}

int Status::strengthDefault() const
{
	return 10;
}
int Status::wisdomDefault() const
{
	return 10;
}
int Status::agilityDefault() const
{
	return 10;
}
int Status::inteligenceDefault() const
{
	return 10;
}
void Status::loadDefaults()
{
	setStrength(10);
	setWisdom(10);
	setAgility(10);
	setInteligence(10);
}
Equipment::Equipment(QObject *parent) : QObject(parent)
{
}

void Equipment::loadDefaults()
{
}
Character::Character(QObject *parent) : QObject(parent),
	_status(new Status(this)),
	_equipment(new Equipment(this)),
	_name(QStringLiteral("John Who")),
	_age(18),
	_gold(100),
	_playerRace(HUMAN)
{
}

QString Character::name() const
{
	return _name;
}

int Character::age() const
{
	return _age;
}

int Character::gold() const
{
	return _gold;
}

race Character::playerRace() const
{
	return _playerRace;
}

Status* Character::status() const
{
	return _status;
}

Equipment* Character::equipment() const
{
	return _equipment;
}

void Character::setName(const QString& value)
{
	if (_name==value) {
		return;
	}
	if (nameRule && !nameRule(value)) {
		return;
	}
	_name = value;
	Q_EMIT nameChanged(value);
}

void Character::setAge(int value)
{
	if (_age==value) {
		return;
	}
	if (ageRule && !ageRule(value)) {
		return;
	}
	_age = value;
	Q_EMIT ageChanged(value);
}

void Character::setGold(int value)
{
	if (_gold==value) {
		return;
	}
	if (goldRule && !goldRule(value)) {
		return;
	}
	_gold = value;
	Q_EMIT goldChanged(value);
}

void Character::setPlayerRace(const race& value)
{
	if (_playerRace==value) {
		return;
	}
	if (playerRaceRule && !playerRaceRule(value)) {
		return;
	}
	_playerRace = value;
	Q_EMIT playerRaceChanged(value);
}

void Character::setNameRule(std::function<bool(QString)> rule)
{
	nameRule = rule;
}

void Character::setAgeRule(std::function<bool(int)> rule)
{
	ageRule = rule;
}

void Character::setGoldRule(std::function<bool(int)> rule)
{
	goldRule = rule;
}

void Character::setPlayerRaceRule(std::function<bool(race)> rule)
{
	playerRaceRule = rule;
}

QString Character::nameDefault() const
{
	return QStringLiteral("John Who");
}
int Character::ageDefault() const
{
	return 18;
}
int Character::goldDefault() const
{
	return 100;
}
race Character::playerRaceDefault() const
{
	return HUMAN;
}
void Character::loadDefaults()
{
	_status->loadDefaults();
	_equipment->loadDefaults();
	setName(QStringLiteral("John Who"));
	setAge(18);
	setGold(100);
	setPlayerRace(HUMAN);
}
Preferences::Preferences(QObject *parent) : QObject(parent),
	_character(new Character(this))
{
	load();
}

QString Preferences::name() const
{
	return _name;
}

int Preferences::age() const
{
	return _age;
}

Character* Preferences::character() const
{
	return _character;
}

void Preferences::setName(const QString& value)
{
	if (_name==value) {
		return;
	}
	if (nameRule && !nameRule(value)) {
		return;
	}
	_name = value;
	Q_EMIT nameChanged(value);
}

void Preferences::setAge(int value)
{
	if (_age==value) {
		return;
	}
	if (ageRule && !ageRule(value)) {
		return;
	}
	_age = value;
	Q_EMIT ageChanged(value);
}

void Preferences::setNameRule(std::function<bool(QString)> rule)
{
	nameRule = rule;
}

void Preferences::setAgeRule(std::function<bool(int)> rule)
{
	ageRule = rule;
}

void Preferences::loadDefaults()
{
	_character->loadDefaults();
}
void Preferences::sync()
{
	KSharedConfigPtr internal_config = KSharedConfig::openConfig();

	KConfigGroup characterGroup = internal_config->group("Character");

	KConfigGroup statusGroup = characterGroup.group("Status");
	if (character()->status()->strength() == character()->status()->strengthDefault()){
		statusGroup.deleteEntry("strength");
	} else { 
		statusGroup.writeEntry("strength",character()->status()->strength());
	}
	if (character()->status()->wisdom() == character()->status()->wisdomDefault()){
		statusGroup.deleteEntry("wisdom");
	} else { 
		statusGroup.writeEntry("wisdom",character()->status()->wisdom());
	}
	if (character()->status()->agility() == character()->status()->agilityDefault()){
		statusGroup.deleteEntry("agility");
	} else { 
		statusGroup.writeEntry("agility",character()->status()->agility());
	}
	if (character()->status()->inteligence() == character()->status()->inteligenceDefault()){
		statusGroup.deleteEntry("inteligence");
	} else { 
		statusGroup.writeEntry("inteligence",character()->status()->inteligence());
	}

	KConfigGroup equipmentGroup = characterGroup.group("Equipment");
	if (character()->name() == character()->nameDefault()){
		characterGroup.deleteEntry("name");
	} else { 
		characterGroup.writeEntry("name",character()->name());
	}
	if (character()->age() == character()->ageDefault()){
		characterGroup.deleteEntry("age");
	} else { 
		characterGroup.writeEntry("age",character()->age());
	}
	if (character()->gold() == character()->goldDefault()){
		characterGroup.deleteEntry("gold");
	} else { 
		characterGroup.writeEntry("gold",character()->gold());
	}
	if (character()->playerRace() == character()->playerRaceDefault()){
		characterGroup.deleteEntry("player_race");
	} else { 
		characterGroup.writeEntry("player_race",(int) character()->playerRace());
	}
	if (name() == nameDefault()){
		internal_config.deleteEntry("name");
	} else { 
		internal_config.writeEntry("name",name());
	}
	if (age() == ageDefault()){
		internal_config.deleteEntry("age");
	} else { 
		internal_config.writeEntry("age",age());
	}
}

void Preferences::load()
{
	KSharedConfigPtr internal_config = KSharedConfig::openConfig();

	KConfigGroup characterGroup = internal_config->group("Character");

	KConfigGroup statusGroup = characterGroup.group("Status");
	character()->status()->setStrength(statusGroup.readEntry<int>("strength", 10));
	character()->status()->setWisdom(statusGroup.readEntry<int>("wisdom", 10));
	character()->status()->setAgility(statusGroup.readEntry<int>("agility", 10));
	character()->status()->setInteligence(statusGroup.readEntry<int>("inteligence", 10));

	KConfigGroup equipmentGroup = characterGroup.group("Equipment");
	character()->setName(characterGroup.readEntry<QString>("name", QStringLiteral("John Who")));
	character()->setAge(characterGroup.readEntry<int>("age", 18));
	character()->setGold(characterGroup.readEntry<int>("gold", 100));
	character()->setPlayerRace((race)characterGroup.readEntry<race>("player_race", HUMAN));
	setName(internal_config.readEntry<QString>("name"));
	setAge(internal_config.readEntry<int>("age"));
}

Preferences* Preferences::self()
{
	static Preferences s;
	return &s;
}
