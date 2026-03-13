#include "rcmodel.h"
#include <QDebug>

RCModel::RCModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int RCModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_products.count();
}

QVariant RCModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_products.count())
        return QVariant();

    const Product &product = m_products[index.row()];

    switch (role) {
    case IdRole:          // ДОБАВЬТЕ ЭТО
        return product.id;
    case NameRole:
        return product.name;
    case SetRateRole:
        return product.setRate;
    case SmoothRateRole:
        return product.smoothRate;
    case ActualRateRole:
        return product.actualRate;
    case QuantityRole:
        return product.quantity;
    case PWMRole:
        return product.pwm;
    case IsActiveRole:
        return product.isActive;
    default:
        return QVariant();
    }
}

QVariantMap RCModel::get(int index) const
{
    QVariantMap map;
    if (index >= 0 && index < m_products.count()) {
        const Product &product = m_products[index];
        map["productId"] = product.id;           // ДОБАВЬТЕ
        map["productName"] = product.name;
        map["productSetRate"] = product.setRate;
        map["productSmoothRate"] = product.smoothRate;
        map["productActualRate"] = product.actualRate;
        map["productQuantity"] = product.quantity;
        map["productPWM"] = product.pwm;
        map["productIsActive"] = product.isActive;
    }
    return map;
}

QHash<int, QByteArray> RCModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "productId";
    roles[NameRole] = "productName";
    roles[SetRateRole] = "productSetRate";
    roles[SmoothRateRole] = "productSmoothRate";
    roles[ActualRateRole] = "productActualRate";
    roles[QuantityRole] = "productQuantity";
    roles[PWMRole] = "productPWM";
    roles[IsActiveRole] = "productIsActive";
    return roles;
}

int RCModel::getProductId(int index) const
{
    if (index >= 0 && index < m_products.count()) {
        return m_products[index].id;
    }
    return -1;
}

void RCModel::setProducts(const QVector<Product> &products)
{
    beginResetModel();
    m_products = products;
    endResetModel();
    emit countChanged();
}

void RCModel::addProduct(const Product &product)
{
    beginInsertRows(QModelIndex(), m_products.count(), m_products.count());
    m_products.append(product);
    endInsertRows();
    emit countChanged();
}

void RCModel::clear()
{
    beginResetModel();
    m_products.clear();
    endResetModel();
    emit countChanged();
}

void RCModel::updateSmoothRate(int index, double newRate)
{
    if (index < 0 || index >= m_products.count()) return;

    // Проверяем, изменилось ли значение
    if (qFuzzyCompare(m_products[index].smoothRate, newRate))
        return;

    m_products[index].smoothRate = newRate;
    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex, {SmoothRateRole});
    emit smoothRateChanged(index, newRate);  // Добавьте эту строку
}

void RCModel::updateActualRate(int index, double newRate)
{
    if (index < 0 || index >= m_products.count()) return;

    // Проверяем, изменилось ли значение
    if (qFuzzyCompare(m_products[index].actualRate, newRate))
        return;

    m_products[index].actualRate = newRate;
    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex, {ActualRateRole});
    emit actualRateChanged(index, newRate);  // Добавьте эту строку
}

void RCModel::updateIsActive(int index, bool isActive)
{
    if (index < 0 || index >= m_products.count()) return;

    if (m_products[index].isActive == isActive)
        return;

    m_products[index].isActive = isActive;
    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex, {IsActiveRole});
    emit productActiveChanged(index, isActive);  // Добавьте эту строку
}

void RCModel::updateSetRate(int index, double newRate)
{
    if (index < 0 || index >= m_products.count()) return;

    m_products[index].setRate = newRate;
    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex, {SetRateRole});
    emit productRateChanged(index, newRate);
}

void RCModel::updateName(int index, const QString &name)
{
    if (index < 0 || index >= m_products.count()) return;

    m_products[index].name = name;
    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex, {NameRole});
}

void RCModel::updateQuantity(int index, double newRate)
{
    if (index < 0 || index >= m_products.count()) return;

    m_products[index].quantity = newRate;
    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex, {QuantityRole});
    emit quantityChanged(index, newRate);
}

void RCModel::updatePWM(int index, int pwm)
{
    if (index < 0 || index >= m_products.count()) return;

    m_products[index].pwm = pwm;
    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex, {PWMRole});
    emit pwmChanged(index, pwm);
}
