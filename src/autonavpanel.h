#ifndef AUTONAVPANEL_H
#define AUTONAVPANEL_H

#include <QWidget>
#include <QSerialPort>
#include <QTimer>

namespace Ui { class AutoNavPanel; }

// include the existing header which defines InnerGpsCache_t if available
// If your project defines InnerGpsCache_t in a different header, change this include accordingly.
extern "C"
{
    // forward-declare the struct members used in assembly
    typedef struct
    {
        double lon;
        double lat;
        double hMSL;
        double velE;
        double velN;
        double velD;
        double heading; // radians
        double pDOP;
        struct
        {
            int year;
            uint8_t month, day, hour, min, sec;
            double msec;
        } time;
        double baroAlt;
        double trueAirSpeed;
    } InnerGpsCache_t;
}

// forward declarations from existing code
//struct InnerGpsCache_t;

class AutoNavPanel : public QWidget
{
    Q_OBJECT
public:
    explicit AutoNavPanel(QSerialPort* port, QWidget *parent = nullptr);
    ~AutoNavPanel();

    void setSerialPort(QSerialPort* port);

private slots:
    void onStartClicked();
    void onStopClicked();
    void onSendOnceClicked();
    void onTimerTimeout();

private:
    void assembleAndSendFrame();

    QSerialPort* m_port;
    QTimer m_timer;

    // UI controls are created programmatically to avoid .ui file
    QWidget* buildControls();

    // configuration state
    uint8_t m_installErrorCorrection = 0;
    bool m_headingValid = false;
    bool m_baselineLengthValid = false;
    bool m_gnssDataValid = false;
    bool m_atmosphereDataValid = false;
    uint8_t m_gnssCombinationForbid = 0;

    double m_PitchCorrection = 0;
    double m_RollCorrection = 0;
    double m_YawCorrection = 0;

    uint8_t m_gnssWorkState = 0;

    bool m_positionVelocityValid = false;
    bool m_utcTimeValid = false;

    double m_baselineLength = 0;

    bool m_AtmosphereHeightDataValid = false;
    bool m_TrueAirSpeedDataValid = false;


    double m_GnssYawCorrection = 0;
    double m_GnssPitchCorrection = 0;
    
    int m_intervalMs = 1000;
    uint8_t m_frameCnt = 0;
    // access extern gps cache
    InnerGpsCache_t* m_gpsCache = nullptr;
};

#endif // AUTONAVPANEL_H
