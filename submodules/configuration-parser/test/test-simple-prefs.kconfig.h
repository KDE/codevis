#pragma once

// DO NOT EDIT THIS FILE
// This file was automatically generated by configuration-parser
// There will be a .conf file somewhere which was used to generate this file
// See https://github.com/tcanabrava/configuration-parser

#include <functional>
#include <QObject>

#include <QString>


class Status : public QObject {
	Q_OBJECT
	Q_PROPERTY(int strength READ strength WRITE setStrength NOTIFY strengthChanged)
	Q_PROPERTY(int wisdom READ wisdom WRITE setWisdom NOTIFY wisdomChanged)
	Q_PROPERTY(int agility READ agility WRITE setAgility NOTIFY agilityChanged)
	Q_PROPERTY(int inteligence READ inteligence WRITE setInteligence NOTIFY inteligenceChanged)

public:
	Status(QObject *parent = 0);
	 void loadDefaults();
	int strength() const;
	void setStrengthRule(std::function<bool(int)> rule);
	int strengthDefault() const;
	int wisdom() const;
	void setWisdomRule(std::function<bool(int)> rule);
	int wisdomDefault() const;
	int agility() const;
	void setAgilityRule(std::function<bool(int)> rule);
	int agilityDefault() const;
	int inteligence() const;
	void setInteligenceRule(std::function<bool(int)> rule);
	int inteligenceDefault() const;

public:
	 Q_SLOT void setStrength(int value);
	 Q_SLOT void setWisdom(int value);
	 Q_SLOT void setAgility(int value);
	 Q_SLOT void setInteligence(int value);
	 Q_SIGNAL void strengthChanged(int value);
	 Q_SIGNAL void wisdomChanged(int value);
	 Q_SIGNAL void agilityChanged(int value);
	 Q_SIGNAL void inteligenceChanged(int value);

private:
	int _strength;
	std::function<bool(int)> strengthRule;
	int _wisdom;
	std::function<bool(int)> wisdomRule;
	int _agility;
	std::function<bool(int)> agilityRule;
	int _inteligence;
	std::function<bool(int)> inteligenceRule;
};

class Equipment : public QObject {
	Q_OBJECT

public:
	Equipment(QObject *parent = 0);
	 void loadDefaults();
};

class Character : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
	Q_PROPERTY(int age READ age WRITE setAge NOTIFY ageChanged)
	Q_PROPERTY(int gold READ gold WRITE setGold NOTIFY goldChanged)
	Q_PROPERTY(race player_race READ playerRace WRITE setPlayerRace NOTIFY playerRaceChanged)
Q_PROPERTY(QObject* status MEMBER _status CONSTANT)
Q_PROPERTY(QObject* equipment MEMBER _equipment CONSTANT)

public:
	Character(QObject *parent = 0);
	 void loadDefaults();
	Status *status() const;
	Equipment *equipment() const;
	QString name() const;
	void setNameRule(std::function<bool(QString)> rule);
	QString nameDefault() const;
	int age() const;
	void setAgeRule(std::function<bool(int)> rule);
	int ageDefault() const;
	int gold() const;
	void setGoldRule(std::function<bool(int)> rule);
	int goldDefault() const;
	race playerRace() const;
	void setPlayerRaceRule(std::function<bool(race)> rule);
	race playerRaceDefault() const;

public:
	 Q_SLOT void setName(const QString& value);
	 Q_SLOT void setAge(int value);
	 Q_SLOT void setGold(int value);
	 Q_SLOT void setPlayerRace(const race& value);
	 Q_SIGNAL void nameChanged(const QString& value);
	 Q_SIGNAL void ageChanged(int value);
	 Q_SIGNAL void goldChanged(int value);
	 Q_SIGNAL void playerRaceChanged(const race& value);

private:
	QString _name;
	std::function<bool(QString)> nameRule;
	int _age;
	std::function<bool(int)> ageRule;
	int _gold;
	std::function<bool(int)> goldRule;
	race _playerRace;
	std::function<bool(race)> playerRaceRule;
	Status *_status;
	Equipment *_equipment;
};

class Preferences : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
	Q_PROPERTY(int age READ age WRITE setAge NOTIFY ageChanged)
Q_PROPERTY(QObject* character MEMBER _character CONSTANT)

public:
	void sync();
	void load();
	static Preferences* self();
	 void loadDefaults();
	Character *character() const;
	QString name() const;
	void setNameRule(std::function<bool(QString)> rule);
	QString nameDefault() const;
	int age() const;
	void setAgeRule(std::function<bool(int)> rule);
	int ageDefault() const;

public:
	 Q_SLOT void setName(const QString& value);
	 Q_SLOT void setAge(int value);
	 Q_SIGNAL void nameChanged(const QString& value);
	 Q_SIGNAL void ageChanged(int value);

private:
	QString _name;
	std::function<bool(QString)> nameRule;
	int _age;
	std::function<bool(int)> ageRule;
	Character *_character;

private:
	Preferences(QObject *parent = 0);
};

