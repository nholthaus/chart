/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Charts module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "chartView.h"
#include <QtGui/QResizeEvent>
#include <QtWidgets/QGraphicsScene>
#include <QChart>
#include <QLineSeries>
#include <QSplineSeries>
#include <QLegendMarker>
#include <QtWidgets/QGraphicsTextItem>
#include "callout.h"
#include "chart.h"
#include "lineSeries.h"
#include <QtGui/QMouseEvent>

ChartView::ChartView(Chart* chart, QWidget *parent)
    : QChartView(chart, parent),
      m_coordX(0),
      m_coordY(0),
      m_chart(chart),
      m_tooltip(0),
	  m_markerSignalMapper(new QSignalMapper)
{
    setDragMode(QGraphicsView::NoDrag);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // chart
    m_chart->setAcceptHoverEvents(true);

    setRenderHint(QPainter::Antialiasing);

	this->setMouseTracking(true);

	// lambdas used in connections
	auto connectSeries = [this](QAbstractSeries* series)
	{
		connect(series, SIGNAL(leftClicked(LineSeries*)), this, SLOT(keepCallout(LineSeries*)));
		connect(series, SIGNAL(hovered(QPointF, bool)), this, SLOT(tooltip(QPointF, bool)));
		foreach(QLegendMarker* marker, this->m_chart->legend()->markers())
		{
			m_markerSignalMapper->setMapping(marker, marker);
			connect(marker, SIGNAL(clicked()), m_markerSignalMapper, SLOT(map()), Qt::UniqueConnection);
			connect(m_markerSignalMapper, SIGNAL(mapped(QObject*)), this, SLOT(toggleSeriesVisibility(QObject*)), Qt::UniqueConnection);
		}
	};

	// Make connections
	foreach(auto* series, m_chart->series())
		connectSeries(series);

	connect(m_chart, &Chart::seriesAdded, [this, connectSeries](QAbstractSeries* series)
	{
		connectSeries(series);
	});

	connect(this->scene(), &QGraphicsScene::selectionChanged, this, [this]()
	{
		foreach(auto item, this->scene()->selectedItems())
		{
			auto callout = dynamic_cast<Callout*>(item);
			if (callout)
				raiseCallout(callout);
		}
	});

	connect(m_chart, &Chart::zoomed, this, [this](QRectF zoomRect)
	{
		m_zoomRect = zoomRect;
		// show the callout only if it's inside the current zoom rect and the series
		// it's attached to is also visible
		foreach(auto callout, m_callouts)
		{
			if (m_zoomRect.contains(callout->anchor()))
			{
				foreach(auto series, m_chart->series())
				{
					if(series->name() == callout->seriesName() && series->isVisible())
						callout->show();
				}
			}
			else
				callout->hide();
		}
	});
}

void ChartView::resizeEvent(QResizeEvent *event)
{
    if (scene()) {
        scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));
         m_chart->resize(event->size());
         foreach (Callout *callout, m_callouts)
             callout->updateGeometry();
    }
    QChartView::resizeEvent(event);
}

// --------------------------------------------------------------------------------
// 	FUNCTION: wheelEvent (public )
// --------------------------------------------------------------------------------
void ChartView::wheelEvent(QWheelEvent *event)
{
	// A positive delta indicates that the wheel was
	// rotated forwards away from the user; a negative
	// value indicates that the wheel was rotated
	// backwards toward the user.
	// Most mouse types work in steps of 15 degrees,
	// in which case the delta value is a multiple
	// of 120 (== 15 * 8).
	if (event->buttons() != Qt::MiddleButton)
	{
		if (m_zoomRect.isEmpty())
			m_zoomRect = m_chart->bounds();

		auto deltaX = m_zoomRect.width() / 4;
		auto deltaY = m_zoomRect.height() / 4;

		if (event->delta() > 0)
		{
			// zoom in using the same methodology as the current rubber band
			if(this->rubberBand() == QChartView::RectangleRubberBand)
				m_chart->zoom(m_zoomRect.adjusted(deltaX, deltaY, -deltaX, -deltaY));
			else if (this->rubberBand() == QChartView::HorizontalRubberBand)
				m_chart->zoom(m_zoomRect.adjusted(deltaX, 0, -deltaX, 0));
			else if(this->rubberBand() == QChartView::VerticalRubberBand)
				m_chart->zoom(m_zoomRect.adjusted(0, deltaY, 0, -deltaY));
		}
		else
		{
			// zoom out using the same methodology as the current rubber band
			if (this->rubberBand() == QChartView::RectangleRubberBand)
				m_chart->zoom(m_zoomRect.adjusted(2 * -deltaX, 2 * -deltaY, 2 * deltaX, 2 * deltaY));
			else if (this->rubberBand() == QChartView::HorizontalRubberBand)
				m_chart->zoom(m_zoomRect.adjusted(2 * -deltaX, 0, 2 * deltaX, 0));
			else if (this->rubberBand() == QChartView::VerticalRubberBand)
				m_chart->zoom(m_zoomRect.adjusted(0, 2 * -deltaY, 0, 2 * deltaY));
		}
	}

	QChartView::wheelEvent(event);
}


// --------------------------------------------------------------------------------
// 	FUNCTION: keyPressEvent (public )
// --------------------------------------------------------------------------------
void ChartView::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Delete)
	{
		foreach(Callout* callout, m_callouts)
		{
			if (callout->isSelected())
			{
				m_callouts.removeOne(callout);
				delete callout;
			}
		}
	}
	if (event->key() == Qt::Key_Escape)
	{
		foreach(Callout* callout, m_callouts)
		{
			callout->setSelected(false);
		}
	}
}


// --------------------------------------------------------------------------------
// 	FUNCTION: mousePressEvent (public )
// --------------------------------------------------------------------------------
void ChartView::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		// unselect any callouts if user clicks outside the plot area
		foreach(Callout* callout, m_callouts)
		{
			callout->setSelected(false);
		}
	}
	if (event->button() == Qt::RightButton)
	{
		// right click deletes callouts if they are under the mouse
		foreach(Callout* callout, m_callouts)
		{
			if (callout->isUnderMouse())
			{
				m_callouts.removeOne(callout);
				delete callout;
				return;
			}
		}

		// otherwise right click resets the zoom level
		m_chart->zoomReset();
		foreach(Callout* callout, m_callouts)
			callout->update();

		return;
	}
	if (event->button() == Qt::MiddleButton)
	{
		QApplication::setOverrideCursor(QCursor(Qt::SizeAllCursor));
		m_lastMousePos = event->pos();
		event->accept();
	}


	QChartView::mousePressEvent(event);
}

// --------------------------------------------------------------------------------
// 	FUNCTION: mouseMoveEvent (public )
// --------------------------------------------------------------------------------
void ChartView::mouseMoveEvent(QMouseEvent *event)
{
	// pan the chart with a middle mouse drag
	if (event->buttons() & Qt::MiddleButton)
	{
		auto dPos = m_chart->mapToValue(event->pos()) - m_chart->mapToValue(m_lastMousePos);

		if (this->rubberBand() == QChartView::RectangleRubberBand)
			m_chart->zoom(m_zoomRect.translated(-dPos.x(), -dPos.y()));
		else if (this->rubberBand() == QChartView::HorizontalRubberBand)
			m_chart->zoom(m_zoomRect.translated(-dPos.x(), 0));
		else if (this->rubberBand() == QChartView::VerticalRubberBand)
			m_chart->zoom(m_zoomRect.translated(0, -dPos.y()));

		m_lastMousePos = event->pos();
		event->accept();
	}

	QChartView::mouseMoveEvent(event);
}

// --------------------------------------------------------------------------------
// 	FUNCTION: mouseReleaseEvent (public )
// --------------------------------------------------------------------------------
void ChartView::mouseReleaseEvent(QMouseEvent *event)
{
	QApplication::restoreOverrideCursor();
	if (event->button() == Qt::RightButton)
	{
		return;
	}

	QChartView::mouseReleaseEvent(event);
}

void ChartView::keepCallout(LineSeries* series)
{
    m_callouts.append(m_tooltip);
	m_tooltip->setZValue(11);
	connect(m_tooltip, &Callout::dropped, this, &ChartView::raiseCallout);

    m_tooltip = new Callout(m_chart);
	m_tooltip->setVisible(false);
}

void ChartView::toggleSeriesVisibility(QObject* obj)
{
	QLegendMarker* marker = qobject_cast<QLegendMarker*>(obj);
	if(marker)
	{
		bool val = !marker->series()->isVisible();
		if(val)
		{
			marker->setLabel(marker->series()->name());
		}
		else
		{
			marker->setLabel(marker->series()->name() + QString(" (hidden)"));
		}
		marker->series()->setVisible(val);
		marker->setVisible(true);

		foreach(auto callout, m_callouts)
		{
			if (callout->seriesName() == marker->series()->name())
				callout->setVisible(val);
		}
	}
}

// --------------------------------------------------------------------------------
// 	FUNCTION: raiseCallout (public )
// --------------------------------------------------------------------------------
void ChartView::raiseCallout(Callout* callout /*= nullptr*/)
{
	callout->setZValue(12);
	foreach(auto otherCallout, m_callouts)
	{
		if (callout != otherCallout && callout->collidesWithItem(otherCallout))
		{
			otherCallout->setZValue(11);
		}
	}
}

void ChartView::tooltip(QPointF point, bool state)
{
	auto series = dynamic_cast<LineSeries*>(this->sender());

	if (m_tooltip == 0)
	{
		m_tooltip = new Callout(m_chart);
	}

    if (state) {
        m_tooltip->setText(QString("X: %1 \nY: %2 ").arg(point.x()).arg(point.y()));
        m_tooltip->setAnchor(point);
        m_tooltip->updateGeometry();
		m_tooltip->setBrush(QBrush(series->pen().color()));
		m_tooltip->setSeriesName(series->name());
		m_tooltip->setZValue(13);
        m_tooltip->show();
    } else {
        m_tooltip->hide();
    }
}

// --------------------------------------------------------------------------------
// 	FUNCTION: clearCallouts (public )
// --------------------------------------------------------------------------------
void ChartView::clearCallouts()
{
	foreach(Callout* callout, m_callouts)
	{
		m_callouts.removeOne(callout);
		delete callout;
	}
}