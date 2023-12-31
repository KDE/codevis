#pragma once

// DO NOT EDIT THIS FILE
// This file was automatically generated by configuration-parser
// There will be a .conf file somewhere which was used to generate this file
// See https://github.com/tcanabrava/configuration-parser

#include <functional>
#include <QObject>

#include <QPoint>
#include <QString>


class Preferences : public QObject {
	Q_OBJECT
	Q_PROPERTY(QPoint point READ point WRITE setPoint NOTIFY pointChanged)
	Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
	Q_PROPERTY(int age READ age WRITE setAge NOTIFY ageChanged)

public:
	void sync();
	void load();
	static Preferences* self();
	 void loadDefaults();
	QPoint point() const;
	void setPointRule(std::function<bool(QPoint)> rule);
	QPoint pointDefault() const;
	QString name() const;
	void setNameRule(std::function<bool(QString)> rule);
	QString nameDefault() const;
	int age() const;
	void setAgeRule(std::function<bool(int)> rule);
	int ageDefault() const;

public:
	 Q_SLOT void setPoint(const QPoint& value);
	 Q_SLOT void setName(const QString& value);
	 Q_SLOT void setAge(int value);
	 Q_SIGNAL void pointChanged(const QPoint& value);
	 Q_SIGNAL void nameChanged(const QString& value);
	 Q_SIGNAL void ageChanged(int value);

private:
	QPoint _point;
	std::function<bool(QPoint)> pointRule;
	QString _name;
	std::function<bool(QString)> nameRule;
	int _age;
	std::function<bool(int)> ageRule;

private:
	Preferences(QObject *parent = 0);
};

