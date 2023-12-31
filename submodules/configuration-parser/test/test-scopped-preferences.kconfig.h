#pragma once

// DO NOT EDIT THIS FILE
// This file was automatically generated by configuration-parser
// There will be a .conf file somewhere which was used to generate this file
// See https://github.com/tcanabrava/configuration-parser

#include <functional>
#include <QObject>

#include <test1>


class InnerInnerPrefs1 : public QObject {
	Q_OBJECT
	Q_PROPERTY(int blah READ blah WRITE setBlah NOTIFY blahChanged)

public:
	InnerInnerPrefs1(QObject *parent = 0);
	 void loadDefaults();
	int blah() const;
	void setBlahRule(std::function<bool(int)> rule);
	int blahDefault() const;

public:
	 Q_SLOT void setBlah(int value);
	 Q_SIGNAL void blahChanged(int value);

private:
	int _blah;
	std::function<bool(int)> blahRule;
};

class InnerPrefs1 : public QObject {
	Q_OBJECT
Q_PROPERTY(QObject* inner_inner_prefs1 MEMBER _innerInnerPrefs1 CONSTANT)

public:
	InnerPrefs1(QObject *parent = 0);
	 void loadDefaults();
	InnerInnerPrefs1 *innerInnerPrefs1() const;

private:
	InnerInnerPrefs1 *_innerInnerPrefs1;
};

class InnerPrefs2 : public QObject {
	Q_OBJECT

public:
	InnerPrefs2(QObject *parent = 0);
	 void loadDefaults();
};

class Preferences : public QObject {
	Q_OBJECT
Q_PROPERTY(QObject* inner_prefs1 MEMBER _innerPrefs1 CONSTANT)
Q_PROPERTY(QObject* inner_prefs2 MEMBER _innerPrefs2 CONSTANT)

public:
	void sync();
	void load();
	static Preferences* self();
	 void loadDefaults();
	InnerPrefs1 *innerPrefs1() const;
	InnerPrefs2 *innerPrefs2() const;

private:
	InnerPrefs1 *_innerPrefs1;
	InnerPrefs2 *_innerPrefs2;
	Preferences(QObject *parent = 0);
};

