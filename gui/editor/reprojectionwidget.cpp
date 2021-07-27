/*------------------------------------------------------------
 *  reprojectionwidget.cpp
 *  Created:  03. December 2020
 *  Author:   Timo Hüser
 *
 *------------------------------------------------------------*/

#include "reprojectionwidget.hpp"

#include <QFileDialog>

ReprojectionWidget::ReprojectionWidget(QWidget *parent) : QWidget(parent) {
	settings = new QSettings();
	m_colorMap = new ColorMap(ColorMap::Jet);
	QGridLayout *reprojectionlayout = new QGridLayout(this);

	QLabel *reprojectionLabel = new QLabel("Reprojection Tool");
	reprojectionLabel->setFont(QFont("Sans Serif", 12, QFont::Bold));
	toggleSwitch = new Switch(this);
	toggleSwitch->setMaximumSize(55,25);
	toggleSwitch->setEnabled(false);
	connect(toggleSwitch, &Switch::toggled, this, &ReprojectionWidget::switchToggledSlot);

	stackedWidget = new QStackedWidget(this);

	QLabel *loadDatasetFirstLabel = new QLabel("Load Datset First...");
	loadDatasetFirstLabel->setFont(QFont("Sans Serif", 14, QFont::Bold));
	loadDatasetFirstLabel->setAlignment(Qt::AlignCenter);
	stackedWidget->addWidget(loadDatasetFirstLabel);

	calibrationSetup = new QWidget(stackedWidget);
	QGridLayout *calibrationsetuplayout = new QGridLayout(calibrationSetup);
	QGroupBox *infoBox = new QGroupBox(calibrationSetup);
	QGridLayout *infolayout = new QGridLayout(infoBox);
	infolayout->setMargin(4);
	infolayout->setSpacing(10);
	QLabel *infoIcon = new QLabel();
	infoIcon->setPixmap(QIcon::fromTheme("info2").pixmap(QSize(60, 60)));
	QLabel *infoLabel = new QLabel("Select Savefiles containing the intrinsic parameters for all the cameras obtained by calibration.");
	infoLabel->setWordWrap(true);
	infolayout->addWidget(infoIcon,0,0);
	infolayout->addWidget(infoLabel,0,1);

	intrinsicsBox = new QGroupBox("Intrinsic Camera Parameters");
	QGridLayout *intrinsicslayout = new QGridLayout(intrinsicsBox);
	QLabel *intrinsicsPathLabel = new QLabel("Intrinsics Path",intrinsicsBox);
	intrinsicsPathEdit = new QLineEdit(intrinsicsBox);
	intrinsicsPathButton = new QPushButton();
	intrinsicsPathButton->setMinimumSize(30,30);
	intrinsicsPathButton->setIcon(QIcon::fromTheme("folder"));
	connect(intrinsicsPathButton, &QPushButton::clicked, this, &ReprojectionWidget::intrinsicsPathClickedSlot);
	intrinsicslayout->addWidget(intrinsicsPathLabel,0,0);
	intrinsicslayout->addWidget(intrinsicsPathEdit,0,1);
	intrinsicslayout->addWidget(intrinsicsPathButton,0,2);

	extrinsicsBox = new QGroupBox("Extrinsic Camera Parameters");
	QGridLayout *extrinsicslayout = new QGridLayout(extrinsicsBox);
	primaryCombo = new QComboBox(extrinsicsBox);
	extrinsicslayout->addWidget(primaryCombo,0,0);
	QLabel *extrinsicsPathLabel = new QLabel("Extrinsics Path" ,extrinsicsBox);
	extrinsicsPathEdit = new QLineEdit(extrinsicsBox);
	extrinsicsPathButton = new QPushButton();
	extrinsicsPathButton->setMinimumSize(30,30);
	extrinsicsPathButton->setIcon(QIcon::fromTheme("folder"));
	connect(extrinsicsPathButton, &QPushButton::clicked, this, &ReprojectionWidget::extrinsicsPathClickedSlot);
	extrinsicslayout->addWidget(primaryCombo,0,0,1,3);
	extrinsicslayout->addWidget(extrinsicsPathLabel,1,0);
	extrinsicslayout->addWidget(extrinsicsPathEdit,1,1);
	extrinsicslayout->addWidget(extrinsicsPathButton,1,2);
	initReprojectionButton = new QPushButton("Initialise Reprojection Tool");
	connect(initReprojectionButton, &QPushButton::clicked, this, &ReprojectionWidget::initReprojectionClickedSlot);

	calibrationsetuplayout->addWidget(infoBox,0,0);
	calibrationsetuplayout->addWidget(intrinsicsBox,1,0);
	calibrationsetuplayout->addWidget(extrinsicsBox,2,0);
	calibrationsetuplayout->addWidget(initReprojectionButton,3,0,Qt::AlignRight);

	reprojectionController = new QWidget(stackedWidget);
	reprojectioncontrollerlayout = new QGridLayout(reprojectionController);
	reprojectionSettingsBox = new QGroupBox("Settings");
	reprojectionSettingsBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	QGridLayout* reprojectionsettingslayout = new QGridLayout(reprojectionSettingsBox);
	QLabel *minViewsLabel = new QLabel("Min Annotated Views");
	minViewsEdit = new QSpinBox();
	minViewsEdit->setValue(2);
	connect(minViewsEdit, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ReprojectionWidget::minViewsChangedSlot);
	QLabel *errorThresholdLabel = new QLabel("Repro Error Threshold");
	errorThresholdEdit = new QDoubleSpinBox();
	errorThresholdEdit->setValue(10.0);
	connect(errorThresholdEdit, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &ReprojectionWidget::errorThresholdChangedSlot);
	reprojectionChartWidget = new ReprojectionChartWidget(this);

	reprojectionsettingslayout->addWidget(minViewsLabel,0,0);
	reprojectionsettingslayout->addWidget(minViewsEdit,0,1);
	reprojectionsettingslayout->addWidget(errorThresholdLabel,1,0);
	reprojectionsettingslayout->addWidget(errorThresholdEdit,1,1);

	reprojectioncontrollerlayout->addWidget(reprojectionSettingsBox,0,0);
	reprojectioncontrollerlayout->addWidget(reprojectionChartWidget,1,0);

	stackedWidget->addWidget(loadDatasetFirstLabel);
	stackedWidget->addWidget(calibrationSetup);
	stackedWidget->addWidget(reprojectionController);

	reprojectionlayout->addWidget(reprojectionLabel,0,0);
	reprojectionlayout->addWidget(toggleSwitch,0,1);
	reprojectionlayout->addWidget(stackedWidget,1,0,1,2);

	//--- SIGNAL-SLOT Connections ---//
	//-> Incoming Signals

	//<- Outgoing Signals
	connect(this, &ReprojectionWidget::datasetLoaded, reprojectionChartWidget, &ReprojectionChartWidget::datasetLoadedSlot);
	connect(this, &ReprojectionWidget::errorThresholdChanged, reprojectionChartWidget, &ReprojectionChartWidget::errorThresholdChanged);
	//<-> Relayed Signals
}


void ReprojectionWidget::datasetLoadedSlot() {
	primaryCombo->clear();
	m_numCameras = Dataset::dataset->numCameras();
	m_entitiesList = Dataset::dataset->entitiesList();
	m_bodypartsList = Dataset::dataset->bodypartsList();

	for (int cam = 0; cam < m_numCameras; cam++) {
		primaryCombo->addItem(Dataset::dataset->cameraName(cam));
	}

	minViewsEdit->setRange(2, m_numCameras);
	//loadPaths();
	stackedWidget->setCurrentWidget(calibrationSetup);
	//checkIntrinsicsPath();
	//checkExtrinsicsPath();
	for (const auto& entity : Dataset::dataset->entitiesList()) {
		m_reprojectionErrors[entity] = new std::vector<double>(Dataset::dataset->bodypartsList().size());
	}

	switchToggledSlot(false);

	emit datasetLoaded();
}


void ReprojectionWidget::switchToggledSlot(bool toggle) {
	if (toggle) {
		if (Dataset::dataset == nullptr) {
			toggleSwitch->setToggled(false);
			return;
		}
		m_reprojectionActive = true;
		calculateAllReprojections();
		calculateReprojectionSlot(m_currentImgSetIndex, m_currentFrameIndex);
	}
	else {
		m_reprojectionActive = false;
		undoReprojection();
	}
	emit reprojectedPoints(Dataset::dataset->imgSets()[m_currentImgSetIndex], m_currentFrameIndex);
	emit reprojectionToolToggled(toggle);
}

void ReprojectionWidget::intrinsicsPathClickedSlot() {
	QString path = QFileDialog::getExistingDirectory(this, "Select Intrinsics Parameter Path", m_parameterDir.path());
	if (path != "") {
		if (checkIntrinsicsPath(path)) {
			m_parameterDir.setPath(path);
			intrinsicsPathEdit->setText(path);
			m_intrinsicsPathValid = true;
		}
	}
}

void ReprojectionWidget::extrinsicsPathClickedSlot() {
	QString path = QFileDialog::getExistingDirectory(this, "Select Extrnsics Parameter Path", m_parameterDir.path());
	if (path != "") {
		if (checkExtrinsicsPath(path)) {
			m_parameterDir.setPath(path);
			extrinsicsPathEdit->setText(path);
			m_extrinsicsPathValid = true;
		}
	}
}

bool ReprojectionWidget::checkIntrinsicsPath(QString path) {
	for (int cam = 0; cam < m_numCameras; cam++) {
		if (!QFile::exists(path + "/" + "Intrinsics_" + Dataset::dataset->cameraName(cam).split("_").takeLast() + ".yaml")) {
			QErrorMessage *msg = new QErrorMessage();
			msg->showMessage("Intrinsics File \"Intrinsics_" + Dataset::dataset->cameraName(cam) + ".yaml\" does not exist.");
			return false;
		}
	}
	return true;
}

bool ReprojectionWidget::checkExtrinsicsPath(QString path) {
	for (int cam = 0; cam < m_numCameras; cam++) {
		if (cam != primaryCombo->currentIndex() && !QFile::exists(path + "/" + "Camera_Pair_" + primaryCombo->currentText().split("_").takeLast() + "_" + Dataset::dataset->cameraName(cam).split("_").takeLast() + ".yaml")) {
			QErrorMessage *msg = new QErrorMessage();
			msg->showMessage("Extrinsics File \"Camera_Pair_" + primaryCombo->currentText().split("_").takeLast() + "_" + Dataset::dataset->cameraName(cam).split("_").takeLast() + ".yaml\" does not exist.");
			return false;
		}
	}
	return true;
}


void ReprojectionWidget::initReprojectionClickedSlot() {
	if (!checkIntrinsicsPath(intrinsicsPathEdit->text()) || !checkExtrinsicsPath(extrinsicsPathEdit->text())) {
		return;
	}
	//savePaths();
	toggleSwitch->setEnabled(true);
	toggleSwitch->setToggled(true);
	emit reprojectionToolToggled(true);
	m_reprojectionActive = true;
	QList<QString> intrinsicsList;
	for (int cam = 0; cam < m_numCameras; cam++) {
		intrinsicsList.append(intrinsicsPathEdit->text() + "/" + "Intrinsics_" + Dataset::dataset->cameraName(cam).split("_").takeLast() + ".yaml");
	}
	QList<QString> extrinsicsList;
	for (int cam = 0; cam < m_numCameras; cam++) {
			extrinsicsList.append(extrinsicsPathEdit->text() + "/" + "Camera_Pair_" + primaryCombo->currentText().split("_").takeLast() + "_" + Dataset::dataset->cameraName(cam).split("_").takeLast() + ".yaml");
	}
	reprojectionTool = new ReprojectionTool(intrinsicsList, extrinsicsList,primaryCombo->currentIndex());
	stackedWidget->setCurrentWidget(reprojectionController);
	calculateAllReprojections();
	calculateReprojectionSlot(m_currentImgSetIndex, m_currentFrameIndex);
}

void ReprojectionWidget::savePaths() {
	settings->beginGroup("CalibrationPaths");
	settings->setValue("IntrinsicsPath", intrinsicsPathEdit->text());
	int i = 0;
	settings->beginGroup("Intrinsics");
	for (const auto& edit : intrinsicsPathEdits) {
		settings->setValue(Dataset::dataset->cameraName(i++), edit->text());
	}
	settings->endGroup();
	settings->beginGroup("Extrinsics");
	i = 0;
	for (const auto& edit : extrinsicsPathEdits) {
		settings->setValue(primaryCombo->currentText() + "/" + Dataset::dataset->cameraName(i++), edit->text());
	}
	settings->endGroup();
	settings->endGroup();
}


void ReprojectionWidget::loadPaths() {
	settings->beginGroup("CalibrationPaths");
	int i = 0;
	if (!onlyExtrinsics) {
		settings->beginGroup("Intrinsics");
		if(settings->value(Dataset::dataset->cameraName(0)).toString() != "") {
			m_parameterDir.setPath(settings->value(Dataset::dataset->cameraName(0)).toString());
		}
		for (const auto& edit : intrinsicsPathEdits) {
			edit->setText(settings->value(Dataset::dataset->cameraName(i++)).toString());
		}
		settings->endGroup();
		i = 0;
	}
	settings->beginGroup("Extrinsics");
	if(settings->value(primaryCombo->currentText() + "/" + Dataset::dataset->cameraName(0)).toString() != "") {
		m_parameterDir.setPath(settings->value(primaryCombo->currentText() + "/" + Dataset::dataset->cameraName(0)).toString());
	}
	for (const auto& edit : extrinsicsPathEdits) {
		edit->setText(settings->value(primaryCombo->currentText() + "/" + Dataset::dataset->cameraName(i++)).toString());
	}
	settings->endGroup();
	settings->endGroup();
}


void ReprojectionWidget::calculateReprojectionSlot(int currentImgSetIndex, int currentFrameIndex) {
	m_currentImgSetIndex = currentImgSetIndex;
	m_currentFrameIndex = currentFrameIndex;
	if (m_reprojectionActive) {
		for (const auto& entity : m_entitiesList) {
			for (const auto& bodypart : m_bodypartsList) {
				QList<int> camsToUse;
				QList<int> alreadyReprojected;
				QList<QPointF> points;
				int camCounter = 0;
				for (const auto& frame : Dataset::dataset->imgSets()[currentImgSetIndex]->frames) {
					if (frame->keypointMap[entity + "/" + bodypart]->state() == Annotated) {
						camsToUse.append(camCounter);
						points.append(frame->keypointMap[entity + "/" + bodypart]->coordinates());
					}
					else if (frame->keypointMap[entity + "/" + bodypart]->state() == Reprojected) {
						alreadyReprojected.append(camCounter);
					}
					camCounter++;
				}
				if (camsToUse.size() >= m_minViews) {
					cv::Mat X =  reprojectionTool->reconstructPoint3D(points, camsToUse);
					QList<QPointF> reprojectedPoints = reprojectionTool->reprojectPoint(X);
					double reprojectionError = 0;
					for (int cam = 0; cam < reprojectedPoints.size(); cam ++) {
						QSize imgSize = Dataset::dataset->imgSets()[currentImgSetIndex]->frames[cam]->imageDimensions;
						QRectF imgRect(QPoint(0,0), imgSize);
						Keypoint *keypoint = Dataset::dataset->imgSets()[currentImgSetIndex]->frames[cam]->keypointMap[entity + "/" + bodypart];
						if (!camsToUse.contains(cam) && imgRect.contains(reprojectedPoints[cam])) {
							if (keypoint->state() != Suppressed) {
								keypoint->setState(Reprojected);
								keypoint->setCoordinates(reprojectedPoints[cam]);
							}
						}
						else if (keypoint->state() == Annotated) {
							QPointF annotatedPoint = Dataset::dataset->imgSets()[currentImgSetIndex]->frames[cam]->keypointMap[entity + "/" + bodypart]->coordinates();
							QPointF dist = annotatedPoint-reprojectedPoints[cam];
							reprojectionError += sqrt(dist.x()*dist.x()+dist.y()*dist.y())/reprojectedPoints.size();
						}
					}
						(*m_reprojectionErrors[entity])[Dataset::dataset->bodypartsList().indexOf(bodypart)] = reprojectionError;
				}
				else {
					(*m_reprojectionErrors[entity])[Dataset::dataset->bodypartsList().indexOf(bodypart)] = 0;
					for (int cam = 0; cam < Dataset::dataset->numCameras(); cam ++) {
						Keypoint *keypoint = Dataset::dataset->imgSets()[currentImgSetIndex]->frames[cam]->keypointMap[entity + "/" + bodypart];
						if (keypoint->state() == Reprojected) {
							keypoint->setState(NotAnnotated);
						}
					}
				}
			}
		}
		emit reprojectedPoints(Dataset::dataset->imgSets()[currentImgSetIndex], currentFrameIndex);
		emit reprojectionToolToggled(true);
		reprojectionChartWidget->reprojectionErrorsUpdatedSlot(m_reprojectionErrors);
	}
}

void ReprojectionWidget::calculateAllReprojections() {
	if (m_reprojectionActive) {
		for (const auto& imgSet : Dataset::dataset->imgSets()) {
			for (const auto& entity : m_entitiesList) {
				for (const auto& bodypart : m_bodypartsList) {
					QList<int> camsToUse;
					QList<int> alreadyReprojected;
					QList<QPointF> points;
					int camCounter = 0;
					for (const auto& frame : imgSet->frames) {
						if (frame->keypointMap[entity + "/" + bodypart]->state() == Annotated) {
							camsToUse.append(camCounter);
							points.append(frame->keypointMap[entity + "/" + bodypart]->coordinates());
						}
						else if (frame->keypointMap[entity + "/" + bodypart]->state() == Reprojected) {
							alreadyReprojected.append(camCounter);
						}
						camCounter++;
					}
					if (camsToUse.size() >= m_minViews) {
						cv::Mat X =  reprojectionTool->reconstructPoint3D(points, camsToUse);
						QList<QPointF> reprojectedPoints = reprojectionTool->reprojectPoint(X);
						double reprojectionError = 0;
						for (int cam = 0; cam < reprojectedPoints.size(); cam ++) {
							QSize imgSize =imgSet->frames[cam]->imageDimensions;
							QRectF imgRect(QPoint(0,0), imgSize);
							Keypoint *keypoint = imgSet->frames[cam]->keypointMap[entity + "/" + bodypart];
							if (!camsToUse.contains(cam) && imgRect.contains(reprojectedPoints[cam])) {
								if (keypoint->state() != Suppressed) {
									keypoint->setState(Reprojected);
									keypoint->setCoordinates(reprojectedPoints[cam]);
								}
							}
							else if (keypoint->state() == Annotated) {
								QPointF annotatedPoint = imgSet->frames[cam]->keypointMap[entity + "/" + bodypart]->coordinates();
								QPointF dist = annotatedPoint-reprojectedPoints[cam];
								reprojectionError += sqrt(dist.x()*dist.x()+dist.y()*dist.y())/reprojectedPoints.size();
							}
						}
							(*m_reprojectionErrors[entity])[Dataset::dataset->bodypartsList().indexOf(bodypart)] = reprojectionError;
					}
					else {
						(*m_reprojectionErrors[entity])[Dataset::dataset->bodypartsList().indexOf(bodypart)] = 0;
						for (int cam = 0; cam < Dataset::dataset->numCameras(); cam ++) {
							Keypoint *keypoint = imgSet->frames[cam]->keypointMap[entity + "/" + bodypart];
							if (keypoint->state() == Reprojected) {
								keypoint->setState(NotAnnotated);
							}
						}
					}
				}
			}
		}
	}
}

void ReprojectionWidget::undoReprojection() {
	for (auto& imgSet : Dataset::dataset->imgSets()) {
		for (auto& frame : imgSet->frames) {
			for (auto& keypoint : frame->keypoints) {
				if (keypoint->state() == Reprojected) keypoint->setState(NotAnnotated);
			}
		}
	}
}

void ReprojectionWidget::minViewsChangedSlot(int value) {
	m_minViews = value;
	calculateAllReprojections();
	calculateReprojectionSlot(m_currentImgSetIndex, m_currentFrameIndex);
}

void ReprojectionWidget::errorThresholdChangedSlot(double value) {
	m_errorThreshold = value;
	emit errorThresholdChanged(value);
}
