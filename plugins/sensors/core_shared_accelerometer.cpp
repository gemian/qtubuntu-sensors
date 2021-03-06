/*
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "core_shared_accelerometer.h"

#include <QDebug>
#include <QMetaType>

Q_DECLARE_METATYPE(QSharedPointer<QAccelerometerReading>)

core::SharedAccelerometer& core::SharedAccelerometer::instance()
{
    static core::SharedAccelerometer instance;
    return instance;
}

core::SharedAccelerometer::SharedAccelerometer(QObject *parent)
    : QObject(parent),
      m_minDelay(-1),
      m_minValue(0.0),
      m_maxValue(0.0),
      m_resolution(0.0),
      m_available(false)
{
    qRegisterMetaType<QSharedPointer<QAccelerometerReading> >("QSharedPointer<QAccelerometerReading>");
    m_accelerometer = ua_sensors_accelerometer_new();
    if (m_accelerometer == NULL)
        return;

    ua_sensors_accelerometer_set_reading_cb(
                m_accelerometer,
                core::SharedAccelerometer::onAccelerometerReadingCb,
                static_cast<void *>(this));

    // Get the minimum sensor reading delay
    m_minDelay = ua_sensors_accelerometer_get_min_delay(m_accelerometer);

    float value;
    if (ua_sensors_accelerometer_get_min_value(m_accelerometer, &value) == U_STATUS_SUCCESS)
        m_minValue = value;

    if (ua_sensors_accelerometer_get_max_value(m_accelerometer, &value) == U_STATUS_SUCCESS)
        m_maxValue = value;

    if (ua_sensors_accelerometer_get_resolution(m_accelerometer, &value) == U_STATUS_SUCCESS)
        m_resolution = value;

    m_available = true;

}

void core::SharedAccelerometer::start()
{
    if (m_available)
        ua_sensors_accelerometer_enable(m_accelerometer);
}

void core::SharedAccelerometer::stop()
{
    if (m_available)
        ua_sensors_accelerometer_disable(m_accelerometer);
}

qreal core::SharedAccelerometer::getMinDelay() const
{
    return m_minDelay;
}

qreal core::SharedAccelerometer::getMinValue() const
{
    return m_minValue;
}

qreal core::SharedAccelerometer::getMaxValue() const
{
    return m_maxValue;
}

qreal core::SharedAccelerometer::getResolution() const
{
    return m_resolution;
}

void core::SharedAccelerometer::setDelay(quint32 delay_us)
{
    ua_sensors_accelerometer_set_event_rate(m_accelerometer, delay_us * 1000);
}

void core::SharedAccelerometer::onAccelerometerReadingCb(UASAccelerometerEvent *event, void *context)
{
    SharedAccelerometer* ac = static_cast<SharedAccelerometer*>(context);
    if (ac != NULL)
        ac->onAccelerometerReading(event);
}

void core::SharedAccelerometer::onAccelerometerReading(UASAccelerometerEvent *event)
{
    Q_ASSERT(event != NULL);

    // TODO(tvoss): We should rely on an object pool to recycle accelerometer reading
    // instances here. We could use a custom deleter for the shared pointer to put
    // instances that have been successfully delivered to slots back into the pool.
    QSharedPointer<QAccelerometerReading> reading(new QAccelerometerReading());

    float value = -1.;

    if (uas_accelerometer_event_get_acceleration_x(event, &value) == U_STATUS_SUCCESS)
        reading->setX(value);
    if (uas_accelerometer_event_get_acceleration_y(event, &value) == U_STATUS_SUCCESS)
        reading->setY(value);
    if (uas_accelerometer_event_get_acceleration_z(event, &value) == U_STATUS_SUCCESS)
        reading->setZ(value);

    reading->setTimestamp(uas_accelerometer_event_get_timestamp(event));

    Q_EMIT accelerometerReadingChanged(reading);
}
