 /*****************************************************************
 	* File:			  editowidget.hpp
 	* Created: 	  23. October 2020
 	* Author:		  Timo Hueser
 	* Contact: 	  timo.hueser@gmail.com
 	* Copyright:  2021 Timo Hueser
 	* License:    GPL v3.0
 	*****************************************************************/

#ifndef EDITORWIDGET_H
#define EDITORWIDGET_H

#include "globals.hpp"
#include "keypointwidget.hpp"
#include "imageviewer.hpp"
#include "reprojectionwidget.hpp"
#include "datasetcontrolwidget.hpp"

#include <QGridLayout>
#include <QLabel>
#include <QToolBar>
#include <QMouseEvent>
#include <QDrag>
#include <QFrame>
#include <QPushButton>
#include <QSplitter>
#include <QCheckBox>
#include <QTabWidget>
#include <QListWidget>


class EditorWidget : public QWidget {
	Q_OBJECT
	public:
		explicit EditorWidget(QWidget *parent = nullptr);

	signals:
		void datasetLoaded();
		void zoomToggled(bool toggle);
		void panToggled(bool toggle);
		void homeClicked();
		void quitClicked();
		void newSegmentLoaded();
		void frameChanged(int currentImgSetIndex, int currentFrameIndex);
		void imageTranformationChanged(int hueFactor, int saturationFactor, int brightnessFactor);
		void keypointSizeChanged(int value);
		void keypointShapeChanged(const QString& entity, KeypointShape shape);
		void colorMapChanged(const QString& entity, ColorMap::ColorMapType type, QColor color);
		void minViewsChanged(int val);
		void errorThresholdChanged(float val);

	public slots:
		void splitterMovedSlot(int pos, int index);
		void datasetLoadedSlot();
		void frameChangedSlot(int index);
		void imgSetChangedSlot(int index);

	private:
		QSplitter *mainSplitter;
		QSplitter *horizontalSplitter;

		QSplitter *leftSplitter;
		ReprojectionWidget *reprojectionWidget;
		DatasetControlWidget *datasetControlWidget;

		QWidget *imageViewerContainer;
		ImageViewer *imageViewer;


		KeypointWidget *keypointWidget;
		QWidget  *buttonWidget;
		QPushButton *previousButton;
		QPushButton *nextButton;
		QPushButton *zoomButton;
		QPushButton *panButton;
		QPushButton *homeButton;
		QPushButton *previousSetButton;
		QPushButton *nextSetButton;
		QPushButton *quitButton;

		ImgSet *m_currentImgSet;
		int m_currentImgSetIndex;
		int m_currentFrameIndex;

	private slots:
		void previousClickedSlot();
		void nextClickedSlot();
		void zoomToggledSlot(bool);
		void panToggledSlot(bool);
		void homeClickedSlot();
		void previousSetClickedSlot();
		void nextSetClickedSlot();
		void zoomFinishedSlot();
		void panFinishedSlot();
		void quitClickedSlot();
};

#endif
