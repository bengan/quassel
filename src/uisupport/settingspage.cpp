/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "settingspage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QVariant>

#include <QDebug>

#include "uisettings.h"

SettingsPage::SettingsPage(const QString &category, const QString &title, QWidget *parent)
  : QWidget(parent),
    _category(category),
    _title(title),
    _changed(false),
    _autoWidgetsChanged(false)
{
}

void SettingsPage::setChangedState(bool hasChanged_) {
  if(hasChanged_ != _changed) {
    bool old = hasChanged();
    _changed = hasChanged_;
    if(hasChanged() != old)
      emit changed(hasChanged());
  }
}

void SettingsPage::load(QCheckBox *box, bool checked) {
  box->setProperty("StoredValue", checked);
  box->setChecked(checked);
}

bool SettingsPage::hasChanged(QCheckBox *box) {
  return box->property("StoredValue").toBool() == box->isChecked();
}


void SettingsPage::load(QComboBox *box, int index) {
  box->setProperty("StoredValue", index);
  box->setCurrentIndex(index);
}

bool SettingsPage::hasChanged(QComboBox *box) {
  return box->property("StoredValue").toInt() == box->currentIndex();
}

void SettingsPage::load(QSpinBox *box, int value) {
  box->setProperty("StoredValue", value);
  box->setValue(value);
}

bool SettingsPage::hasChanged(QSpinBox *box) {
  return box->property("StoredValue").toInt() == box->value();
}

/*** Auto child widget handling ***/

void SettingsPage::initAutoWidgets() {
  _autoWidgets.clear();

  if(settingsKey().isNull())
    return;

  // find all descendants that should be considered auto widgets
  // we need to climb the QObject tree recursively
  findAutoWidgets(this, &_autoWidgets);

  foreach(QObject *widget, _autoWidgets) {
    if(widget->inherits("QAbstractButton") || widget->inherits("QGroupBox"))
      connect(widget, SIGNAL(toggled(bool)), SLOT(autoWidgetHasChanged()));
    else if(widget->inherits("QLineEdit") || widget->inherits("QTextEdit"))
      connect(widget, SIGNAL(textChanged(const QString &)), SLOT(autoWidgetHasChanged()));
    else if(widget->inherits("QComboBox"))
      connect(widget, SIGNAL(currentIndexChanged(int)), SLOT(autoWidgetHasChanged()));
    else if(widget->inherits("QSpinBox"))
      connect(widget, SIGNAL(valueChanged(int)), SLOT(autoWidgetHasChanged()));
    else
      qWarning() << "SettingsPage::init(): Unknown autoWidget type" << widget->metaObject()->className();
  }
}

void SettingsPage::findAutoWidgets(QObject *parent, QObjectList *autoList) const {
  foreach(QObject *child, parent->children()) {
    if(!child->property("settingsKey").toString().isEmpty())
      autoList->append(child);
    findAutoWidgets(child, autoList);
  }
}

QByteArray SettingsPage::autoWidgetPropertyName(QObject *widget) const {
  QByteArray prop;
  if(widget->inherits("QAbstractButton") || widget->inherits("QGroupBox"))
    prop = "checked";
  else if(widget->inherits("QLineEdit") || widget->inherits("QTextEdit"))
    prop = "text";
  else if(widget->inherits("QComboBox"))
    prop = "currentIndex";
  else if(widget->inherits("QSpinBox"))
    prop = "value";
  else
    qWarning() << "SettingsPage::autoWidgetPropertyName(): Unhandled widget type for" << widget;

  return prop;
}

QString SettingsPage::autoWidgetSettingsKey(QObject *widget) const {
  QString key = widget->property("settingsKey").toString();
  if(key.startsWith('/'))
    key.remove(0, 1);
  else
    key.prepend(settingsKey() + '/');
  return key;
}

void SettingsPage::autoWidgetHasChanged() {
  bool changed_ = false;
  foreach(QObject *widget, _autoWidgets) {
    QVariant curValue = widget->property(autoWidgetPropertyName(widget));
    if(!curValue.isValid())
      qWarning() << "SettingsPage::autoWidgetHasChanged(): Unknown property";

    if(curValue != widget->property("storedValue")) {
      changed_ = true;
      break;
    }
  }

  if(changed_ != _autoWidgetsChanged) {
    bool old = hasChanged();
    _autoWidgetsChanged = changed_;
    if(hasChanged() != old)
      emit changed(hasChanged());
  }
}

void SettingsPage::load() {
  UiSettings s("");
  foreach(QObject *widget, _autoWidgets) {
    QVariant val = s.value(autoWidgetSettingsKey(widget), widget->property("defaultValue"));
    widget->setProperty(autoWidgetPropertyName(widget), val);
    widget->setProperty("storedValue", val);
  }
  bool old = hasChanged();
  _autoWidgetsChanged = _changed = false;
  if(hasChanged() != old)
    emit changed(hasChanged());
}

void SettingsPage::save() {
  UiSettings s("");
  foreach(QObject *widget, _autoWidgets) {
    QVariant val = widget->property(autoWidgetPropertyName(widget));
    widget->setProperty("storedValue", val);
    s.setValue(autoWidgetSettingsKey(widget), val);
  }
  bool old = hasChanged();
  _autoWidgetsChanged = _changed = false;
  if(hasChanged() != old)
    emit changed(hasChanged());
}

void SettingsPage::defaults() {
  foreach(QObject *widget, _autoWidgets) {
    QVariant val = widget->property("defaultValue");
    widget->setProperty(autoWidgetPropertyName(widget), val);
  }
  autoWidgetHasChanged();
}
