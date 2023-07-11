# configuration-parser

This is a simple configuration parser for QSettings or QObject based projects.

purpose of the QObject generator: `Q_PROPERTY` is annoying.

 1. you need to define the property
 2. you need to define the get, set and noify on the header
 3. you need to define the get, set and call nofity in the source

One line of `Q_PROPERTY` will easily be +10 lines in total, making it
cumberstome to program, and if your Object talks to Qml, everything
that Qml touches needs to be `Q_PROPERTY`.

This generator will transform a Qml like object into a pair of header/source with
a default implementation so you can start working faster.

Example: exposedobj.conf

```cpp
ExposedObjectToQml {
    int value
    QString name
}
```

generates the exposedobj.{h,cpp} with:

```cpp
class ExposedObjectToQml {
    Q_OBJECT(int value READ value WRITE setValye NOTIFY valueChanged)
    Q_OBJECT(QString name READ name WRITE setName NOTIFY nameChangeD)

signals:
    void valueChanged(int value);
    void nameChanged(const QString & name);

slots:
    void setValue(int value);
    void setName(const QString & name);

public:
    int value() const;
    QString name() const;

    void setValueRule(lambda-func);
    void setNameRule(lambda-func);
}
```

the lambda-func should should be on this format:

setValueRule([](int value) { return value < 20 && value > 0; };

So you can quickly define what's appropriate for the values

And also the corresponding code in the .cpp file for the get and set functions.

### Purpose:

`QSettings` works nicely but we have a problem with runtime errors as it's impossible for QSettings
to know if you wanna use the "locale" or the "Iocale", it's really easy to commit mistakes using
string based key-value pairs. Also it's hard to know when a value is being saved on disk for the
first time or being loaded, as QSettings tends to make those thigns not explicit.

The parser acts on a Configuration file and generates correct C++ / Qt source code that can be
directly imported into your project, making the usage of QSettings much safer, as it's

- Compile Time Safe (you will get errors if you did something wrong on the configuration file)
- Run time safe (you will get errors if yo pass invalid types to the settings)
- Not possible to have typos on keys anymore
- supports Enums natively (while QSettings doesn't)

On the configuration file, Each level of brackets opens a new configuration group,
and normal c++ objects can be used if QSettings supports them, you can also specify default values
and use includes and enums:

```cpp
#include <QString>

napespace Pref {
    Preferences {
        MainWindow {
            QSize size
            QString title
        }
        Network {
            Proxy {
                QString port
                QString address
            }
            QString username = "untitled"
        }
    }
}
```


This example will create Preferences, MainWindow and Network classes ( I probably should add them into a namespace),
while the Preferences is a singleton, and have all it's children. you can invoke like this:

```cpp
Preferences::self()->mainWindow()->title();
Preferences::self()->network()->proxy()->port();
```

Every configuration has a Changed signal, just like QML, and you can connect on them easily:

```cpp
connect(Preferences::self()->mainWindow(), &MainWindow::titleChanged, [](const QString& s) {
    qCDebug(test) << "Title Changed to" << s;
})
```

## TODO:
watch the cofiguration file on disk and act upon it's changes.
accept includes with ""  and <>, currently just <> is accepted.
