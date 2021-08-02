/*------------------------------------------------------------
 *  calibrationtool.hpp
 *  Created: 30. July 2021
 *  Author:  Timo Hüser
 * Contact: 	timo.hueser@gmail.com
 *------------------------------------------------------------*/

#ifndef CALIBRATIONTOOL_H
#define CALIBRATIONTOOL_H

#include "globals.hpp"
#include "intrinsicscalibrator.hpp"
#include "extrinsicscalibrator.hpp"

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/calib3d.hpp"
#include <string>
#include <vector>
#include <iostream>

class CalibrationTool : public QObject {
	Q_OBJECT
  public:
    explicit CalibrationTool(CalibrationConfig *calibrationConfig);

  public slots:
    void makeCalibrationSet();

  signals:
    void intrinsicsProgress(int counter, int frameCount, int threadNumber);
    void extrinsicsProgress(int counter, int frameCount, int threadNumber);
    void calibrationFinished();

  private:
    CalibrationConfig *m_calibrationConfig;

};




#endif