/*
  propertywidget.cpp

  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2010-2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Volker Krause <volker.krause@kdab.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "propertywidget.h"
#include "ui_propertywidget.h"

#include "ui/methodinvocationdialog.h"
#include "ui/propertyeditor/propertyeditordelegate.h"
#include "ui/propertyeditor/propertyeditorfactory.h"
#include "ui/deferredresizemodesetter.h"

#include "variantcontainermodel.h"
#include "editabletypesmodel.h"

#include "common/objectbroker.h"
#include "common/modelroles.h"
#include "common/metatypedeclarations.h"
#include "common/propertycontrollerinterface.h"
#include <common/propertymodel.h>

#include "kde/krecursivefilterproxymodel.h"

#include <QDebug>
#include <QMenu>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTime>

using namespace GammaRay;

static bool removePage(QTabWidget *tabWidget, QWidget *widget)
{
  const int index = tabWidget->indexOf(widget);
  if (index == -1) {
    return false;
  }

  tabWidget->removeTab(index);
  return true;
}

PropertyWidget::PropertyWidget(QWidget *parent)
  : QWidget(parent),
    m_ui(new Ui_PropertyWidget),
    m_displayState(PropertyWidgetDisplayState::QObject),
    m_controller(0),
    m_newPropertyValue(0)
{
  m_ui->setupUi(this);
}

PropertyWidget::~PropertyWidget()
{
}

void PropertyWidget::setObjectBaseName(const QString &baseName)
{
  m_objectBaseName = baseName;

  QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
  proxy->setDynamicSortFilter(true);
  proxy->setSourceModel(model("properties"));
  m_ui->propertyView->setModel(proxy);
  m_ui->propertyView->sortByColumn(0, Qt::AscendingOrder);
  new DeferredResizeModeSetter(
    m_ui->propertyView->header(), 0, QHeaderView::ResizeToContents);
  m_ui->propertySearchLine->setProxy(proxy);
  m_ui->propertyView->setItemDelegate(new PropertyEditorDelegate(this));
  connect(m_ui->propertyView, SIGNAL(customContextMenuRequested(QPoint)),
          this, SLOT(propertyContextMenu(QPoint)));
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
  connect(m_ui->propertyView, SIGNAL(doubleClicked(QModelIndex)),
          SLOT(onDoubleClick(QModelIndex)));
#endif

  EditableTypesModel *typesModel = new EditableTypesModel(this);
  proxy = new QSortFilterProxyModel(this);
  proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
  proxy->setSourceModel(typesModel);
  proxy->sort(0);
  m_ui->newPropertyType->setModel(proxy);
  connect(m_ui->newPropertyType, SIGNAL(currentIndexChanged(int)),
          this, SLOT(updateNewPropertyValueEditor()));
  updateNewPropertyValueEditor();
  connect(m_ui->newPropertyName, SIGNAL(textChanged(QString)),
          this, SLOT(validateNewProperty()));
  validateNewProperty();
  connect(m_ui->newPropertyButton, SIGNAL(clicked()),
          this, SLOT(addNewProperty()));

  proxy = new QSortFilterProxyModel(this);
  proxy->setDynamicSortFilter(true);
  proxy->setSourceModel(model("methods"));
  m_ui->methodView->setModel(proxy);
  m_ui->methodView->sortByColumn(0, Qt::AscendingOrder);
  m_ui->methodView->setSelectionModel(ObjectBroker::selectionModel(proxy));
  m_ui->methodView->header()->setResizeMode(QHeaderView::ResizeToContents);
  m_ui->methodSearchLine->setProxy(proxy);
  connect(m_ui->methodView, SIGNAL(doubleClicked(QModelIndex)),
          SLOT(methodActivated(QModelIndex)));
  connect(m_ui->methodView, SIGNAL(customContextMenuRequested(QPoint)),
          SLOT(methodConextMenu(QPoint)));
  m_ui->methodLog->setModel(model("methodLog"));

  proxy = new QSortFilterProxyModel(this);
  proxy->setDynamicSortFilter(true);
  proxy->setSourceModel(model("classInfo"));
  m_ui->classInfoView->setModel(proxy);
  m_ui->classInfoView->sortByColumn(0, Qt::AscendingOrder);
  m_ui->classInfoView->header()->setResizeMode(QHeaderView::ResizeToContents);
  m_ui->classInfoSearchLine->setProxy(proxy);

  proxy = new QSortFilterProxyModel(this);
  proxy->setDynamicSortFilter(true);
  proxy->setSourceModel(model("inboundConnections"));
  m_ui->inboundConnectionView->setModel(proxy);
  m_ui->inboundConnectionView->sortByColumn(0, Qt::AscendingOrder);
  m_ui->inboundConnectionSearchLine->setProxy(proxy);

  proxy = new QSortFilterProxyModel(this);
  proxy->setDynamicSortFilter(true);
  proxy->setSourceModel(model("outboundConnections"));
  m_ui->outboundConnectionView->setModel(proxy);
  m_ui->outboundConnectionView->sortByColumn(0, Qt::AscendingOrder);
  m_ui->outboundConnectionSearchLine->setProxy(proxy);

  proxy = new KRecursiveFilterProxyModel(this);
  proxy->setDynamicSortFilter(true);
  proxy->setSourceModel(model("enums"));
  m_ui->enumView->setModel(proxy);
  m_ui->enumView->sortByColumn(0, Qt::AscendingOrder);
  m_ui->enumView->header()->setResizeMode(QHeaderView::ResizeToContents);
  m_ui->enumSearchLine->setProxy(proxy);

  // save back initial tab widgets
  for (int i = 0; i < m_ui->tabWidget->count(); ++i) {
    m_tabWidgets.push_back(qMakePair(m_ui->tabWidget->widget(i), m_ui->tabWidget->tabText(i)));
  }

  if (m_controller) {
    disconnect(m_controller,
               SIGNAL(displayStateChanged(GammaRay::PropertyWidgetDisplayState::State)),
               this, SLOT(setDisplayState(GammaRay::PropertyWidgetDisplayState::State)));
  }
  m_controller =
    ObjectBroker::object<PropertyControllerInterface*>(m_objectBaseName + ".controller");
  connect(m_controller, SIGNAL(displayStateChanged(GammaRay::PropertyWidgetDisplayState::State)),
          this, SLOT(setDisplayState(GammaRay::PropertyWidgetDisplayState::State)));
}

QAbstractItemModel *PropertyWidget::model(const QString &nameSuffix)
{
  return ObjectBroker::model(m_objectBaseName + '.' + nameSuffix);
}

void GammaRay::PropertyWidget::methodActivated(const QModelIndex &index)
{
  if (!index.isValid() || m_displayState != PropertyWidgetDisplayState::QObject) {
    return;
  }
  m_controller->activateMethod();

  const QMetaMethod::MethodType methodType =
    index.data(ObjectMethodModelRole::MetaMethodType).value<QMetaMethod::MethodType>();
  if (methodType == QMetaMethod::Slot) {
    MethodInvocationDialog dlg(this);
    dlg.setArgumentModel(model("methodArguments"));
    if (dlg.exec()) {
      m_controller->invokeMethod(dlg.connectionType());
    }
  }
}

void PropertyWidget::methodConextMenu(const QPoint &pos)
{
  const QModelIndex index = m_ui->methodView->indexAt(pos);
  if (!index.isValid() || m_displayState != PropertyWidgetDisplayState::QObject) {
    return;
  }

  const QMetaMethod::MethodType methodType =
    index.data(ObjectMethodModelRole::MetaMethodType).value<QMetaMethod::MethodType>();
  QMenu contextMenu;
  if (methodType == QMetaMethod::Slot) {
    contextMenu.addAction(tr("Invoke"));
  } else if (methodType == QMetaMethod::Signal) {
    contextMenu.addAction(tr("Connect to"));
  }

  if (contextMenu.exec(m_ui->methodView->viewport()->mapToGlobal(pos))) {
    methodActivated(index);
  }
}

bool PropertyWidget::showTab(const QWidget *widget, PropertyWidgetDisplayState::State state) const
{
  // TODO: this check needs to consider the server-side Qt version!
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  if (widget == m_ui->inboundConnectionTab ||
      widget == m_ui->outboundConnectionTab) {
    return false;
  }
#endif
  switch(state) {
  case PropertyWidgetDisplayState::QObject:
    return true; // show all
  case PropertyWidgetDisplayState::Object:
    if (widget == m_ui->propertyTab) {
      return true;
    }
    break;
  case PropertyWidgetDisplayState::MetaObject:
    if (widget == m_ui->enumTab || widget == m_ui->classInfoTab || widget == m_ui->methodTab) {
      return true;
    }
    break;
  }
  return false;
}

void PropertyWidget::setDisplayState(PropertyWidgetDisplayState::State state)
{
  m_displayState = state;
  QWidget *currentWidget = m_ui->tabWidget->currentWidget();

  // iterate through all tabs, decide for each tab if it gets hidden or not
  typedef QPair<QWidget *, QString> WidgetStringPair;
  Q_FOREACH (const WidgetStringPair &tab, m_tabWidgets) {
    const bool show = showTab(tab.first, state);
    if (show) {
      m_ui->tabWidget->addTab(tab.first, tab.second);
    } else {
      removePage(m_ui->tabWidget, tab.first);
    }
  }

  if (m_ui->tabWidget->indexOf(currentWidget) >= 0) {
    m_ui->tabWidget->setCurrentWidget(currentWidget);
  }

  m_ui->methodLog->setVisible(m_displayState == PropertyWidgetDisplayState::QObject);
}

void PropertyWidget::onDoubleClick(const QModelIndex &index)
{
  if (index.column() != 0) {
    return;
  }

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
  QVariant var = index.sibling(index.row(), 1).data(Qt::EditRole);

  if (!var.canConvert<QVariantList>() && !var.canConvert<QVariantHash>()) {
    return;
  }

  QTreeView *v = new QTreeView;

  VariantContainerModel *m = new VariantContainerModel(v);
  m->setVariant(var);

  v->setModel(m);
  v->show();
#endif
}

static PropertyEditorFactory::TypeId selectedTypeId(QComboBox* box)
{
  return static_cast<PropertyEditorFactory::TypeId>(box->itemData(box->currentIndex(), Qt::UserRole).toInt());
}

void PropertyWidget::updateNewPropertyValueEditor()
{
  delete m_newPropertyValue;

  const PropertyEditorFactory::TypeId type = selectedTypeId(m_ui->newPropertyType);

  m_newPropertyValue = PropertyEditorFactory::instance()->createEditor(type, this);
  m_ui->newPropertyLayout->insertWidget(5, m_newPropertyValue);
  m_ui->newPropertyValueLabel->setBuddy(m_newPropertyValue);
}

void PropertyWidget::validateNewProperty()
{
  Q_ASSERT(m_newPropertyValue);
  m_ui->newPropertyButton->setEnabled(!m_ui->newPropertyName->text().isEmpty());
}

void PropertyWidget::addNewProperty()
{
  const PropertyEditorFactory::TypeId type = selectedTypeId(m_ui->newPropertyType);

  const QByteArray editorPropertyName = PropertyEditorFactory::instance()->valuePropertyName(type);
  const QVariant value = m_newPropertyValue->property(editorPropertyName);
  m_controller->setProperty(m_ui->newPropertyName->text(), value);

  m_ui->newPropertyName->clear();
  updateNewPropertyValueEditor();
}

void PropertyWidget::propertyContextMenu(const QPoint& pos)
{
  const QModelIndex index = m_ui->propertyView->indexAt(pos);
  if (!index.isValid()) {
    return;
  }

  const int actions = index.data(PropertyModel::ActionRole).toInt();
  if (actions == PropertyModel::NoAction)
    return;

  // TODO: check if this is a dynamic property
  QMenu contextMenu;
  if (actions & PropertyModel::Delete) {
    QAction *action = contextMenu.addAction(tr("Remove"));
    action->setData(PropertyModel::Delete);
  }
  if (actions & PropertyModel::Reset) {
    QAction *action = contextMenu.addAction(tr("Reset"));
    action->setData(PropertyModel::Reset);
  }

  if (QAction *action = contextMenu.exec(m_ui->propertyView->viewport()->mapToGlobal(pos))) {
    const QString propertyName = index.sibling(index.row(), 0).data(Qt::DisplayRole).toString();
    switch (action->data().toInt()) {
      case PropertyModel::Delete:
        m_controller->setProperty(propertyName, QVariant());
        break;
      case PropertyModel::Reset:
        m_controller->resetProperty(propertyName);
        break;
    }
  }
}
