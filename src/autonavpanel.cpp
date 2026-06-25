#include "autonavpanel.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QGroupBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QResizeEvent>
#include <QByteArray>
#include <cmath>
#include <vector>
#include <algorithm>



// reference externs from user's code
InnerGpsCache_t InnerGpsCache;
bool OuterSend_HeadingValid;
bool OuterSend_GNSSDataValid;
bool OuterSend_AtmosphereDataValid;
bool OuterSend_AtmosphereHeightDataValid;
bool OuterSend_TrueAirSpeedDataValid;
uint8_t OuterSend_GNSSCombinationForbid;
uint8_t OuterSend_GNSSWorkState;
bool OuterSend_PositionVelocityValid;
bool OuterSend_UTCTimeValid;


// constants used in original code
#define RAD2DEG (180.0/M_PI)

// A small helper widget that lays out label+widget pairs in a responsive grid.
// Each "row" is represented as a vertical block (label above control) and
// blocks are arranged into N columns depending on available width.
class ResponsiveGrid : public QWidget
{
public:
    explicit ResponsiveGrid(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        m_grid = new QGridLayout(this);
        m_grid->setSpacing(8);
        m_grid->setContentsMargins(0,0,0,0);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }

    void addRow(const QString &labelText, QWidget* control)
    {
        // Create a small item widget that stacks label above control
        QWidget* item = new QWidget(this);
        auto v = new QVBoxLayout(item);
        v->setContentsMargins(0,0,0,0);
        QLabel* lbl = new QLabel(labelText, item);
        lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        v->addWidget(lbl);
        v->addWidget(control);
        m_items.push_back(item);
        rearrange();
    }

protected:
    void resizeEvent(QResizeEvent* ev) override
    {
        QWidget::resizeEvent(ev);
        rearrange();
    }

private:
    // void rearrange()
    // {
    //     // clear current layout
    //     QLayoutItem *child;
    //     while ((child = m_grid->takeAt(0)) != nullptr) {
    //         // nothing here; takeAt removes items
    //         delete child;
    //     }

    //     if (m_items.empty()) return;

    //     const int minColumnWidth = 260; // approximate desired width per column
    //     //int w = width();
    //     int w = contentsRect().width();
    //     int columns = std::max(1, w / std::max(1, minColumnWidth));

    //     int row = 0, col = 0;
    //     for (int i = 0; i < (int)m_items.size(); ++i) {
    //         col = i % columns;
    //         row = i / columns;
    //         m_grid->addWidget(m_items[i], row, col);
    //     }
    //     // stretch last column
    //     for (int c = 0; c < columns; ++c) m_grid->setColumnStretch(c, 1);
    //     setStyleSheet("background:red;");
    //     qDebug()
    //         << "width =" << width()
    //         << "contents =" << contentsRect().width()
    //         << "columns =" << columns;
    // }
    void rearrange()
    {
        while (auto item = m_grid->takeAt(0))
            delete item;

        int columns = 5;

        for (int i = 0; i < m_items.size(); ++i)
        {
            int row = i / columns;
            int col = i % columns;

            m_grid->addWidget(m_items[i], row, col);
        }
    }
    QGridLayout* m_grid;
    std::vector<QWidget*> m_items;
};

AutoNavPanel::AutoNavPanel(QSerialPort* port, QWidget *parent)
    : QWidget(parent), m_port(port)
{
    auto layout = new QVBoxLayout(this);
    layout->addWidget(buildControls());

    connect(&m_timer, &QTimer::timeout, this, &AutoNavPanel::onTimerTimeout);
}

AutoNavPanel::~AutoNavPanel()
{
    if (m_timer.isActive()) m_timer.stop();
}

void AutoNavPanel::setSerialPort(QSerialPort* port)
{
    m_port = port;
}

QWidget* AutoNavPanel::buildControls()
{
    QWidget* w = new QWidget(this);
    auto main = new QVBoxLayout(w);

    // Flags group
    QGroupBox* flagsBox = new QGroupBox(tr("Valid Flags"), w);
    auto flagsLayout = new QHBoxLayout(flagsBox);

    // 航向角数据有效性
    QCheckBox *cbHeading = new QCheckBox(tr("Heading Valid"), flagsBox);
    // 装机基线长度有效标志
    QCheckBox *cbGnssBaselineValid = new QCheckBox(tr("GNSS Baseline Valid"), flagsBox);
    // GNSS数据包有效性
    QCheckBox* cbGnss = new QCheckBox(tr("GNSS Data Valid"), flagsBox);
    // 大气数据包有效性
    QCheckBox* cbAtm = new QCheckBox(tr("Atmosphere Data Valid"), flagsBox);
    // 位置速度数据有效
    QCheckBox *cbPosVel = new QCheckBox(tr("Position/Velocity Valid"), flagsBox);
    // UTC时间数据有效
    QCheckBox *cbUTC = new QCheckBox(tr("UTC Time Valid"), flagsBox);
    // 大气高度数据有效性
    QCheckBox *cbAtmosphereHeight = new QCheckBox(tr("Atmosphere Height Valid"), flagsBox);
    // 真空速数据有效性
    QCheckBox *cbTas = new QCheckBox(tr("True Air Speed Valid"), flagsBox);

    flagsLayout->addWidget(cbHeading);
    flagsLayout->addWidget(cbGnssBaselineValid);
    flagsLayout->addWidget(cbGnss);
    flagsLayout->addWidget(cbAtm);
    flagsLayout->addWidget(cbPosVel);
    flagsLayout->addWidget(cbUTC);
    flagsLayout->addWidget(cbAtmosphereHeight);
    flagsLayout->addWidget(cbTas);

    main->addWidget(flagsBox);

    // More flags (responsive grid)
    QGroupBox* moreBox = new QGroupBox(tr("Other Flags"), w);
    auto moreBoxLayout = new QVBoxLayout(moreBox);
    ResponsiveGrid* moreGrid = new ResponsiveGrid(moreBox);
    moreBoxLayout->setContentsMargins(8,8,8,8);
    moreBoxLayout->addWidget(moreGrid);
    // 安装误差修正命令= 00：无操作；= 01：准备修正； = 10：执行修正； = 11：放弃修正。
    QSpinBox *spInstallError = new QSpinBox(moreBox);
    spInstallError->setRange(0,3);
    // 与GNSS组合导航禁止命令
    QSpinBox *spCombForbid = new QSpinBox(moreBox);
    spCombForbid->setRange(0,3);
    // 俯仰修正量 -5°~ +5°
    QSpinBox *spPitch = new QSpinBox(moreBox);
    spPitch->setRange(-5,5);
    // 横滚修正量 -5°~ +5°
    QSpinBox *spRoll = new QSpinBox(moreBox);
    spRoll->setRange(-5,5);
    // 航向修正量 -180°~ +180°
    QSpinBox *spYaw = new QSpinBox(moreBox);
    spYaw->setRange(-180,180);
    // GNSS东向速度
    QSpinBox *spGnssEast = new QSpinBox(moreBox);
    spGnssEast->setRange(-100,100);
    // GNSS北向速度
    QSpinBox *spGnssNorth = new QSpinBox(moreBox);
    spGnssNorth->setRange(-100,100);
    // GNSS天向速度
    QSpinBox *spGnssUp = new QSpinBox(moreBox);
    spGnssUp->setRange(-100,100);
    // GNSS经度 
    QSpinBox *spGnssLon = new QSpinBox(moreBox);
    spGnssLon->setRange(-180,180);
    // GNSS纬度 
    QSpinBox *spGnssLat = new QSpinBox(moreBox);
    spGnssLat->setRange(-90,90);
    // GNSS海拔高
    QSpinBox *spGnssAlt = new QSpinBox(moreBox);
    spGnssAlt->setRange(-1000,1000);
    // UTC时间/年
    QSpinBox *spUTCYear = new QSpinBox(moreBox);
    spUTCYear->setRange(0,9999);
    // UTC时间/月
    QSpinBox *spUTCMonth = new QSpinBox(moreBox);
    spUTCMonth->setRange(1,12);
    // UTC时间/日
    QSpinBox *spUTCDay = new QSpinBox(moreBox);
    spUTCDay->setRange(1,31);
    // UTC时间/时
    QSpinBox *spUTCHour = new QSpinBox(moreBox);
    spUTCHour->setRange(0,23);
    // UTC时间/分
    QSpinBox *spUTCMin = new QSpinBox(moreBox);
    spUTCMin->setRange(0,59);
    // UTC时间/秒
    QSpinBox *spUTCSec = new QSpinBox(moreBox);
    spUTCSec->setRange(0,59);
    // UTC时间/毫秒
    QSpinBox *spUTCMsec = new QSpinBox(moreBox);
    spUTCMsec->setRange(0,999);
    // GNSS_PDOP
    QSpinBox* spGnssPDOP = new QSpinBox(moreBox);
    spGnssPDOP->setRange(0,255);
    // GNSS工作状态
    QSpinBox* spGnssWork = new QSpinBox(moreBox);
    spGnssWork->setRange(0,3);
    // 大气高度
    QSpinBox* spAtmosphereHeight = new QSpinBox(moreBox);
    spAtmosphereHeight->setRange(-1000,1000);
    // 真空速
    QSpinBox* spTas = new QSpinBox(moreBox);
    spTas->setRange(0,1000);
    // 装机GNSS基线长度
    QSpinBox* spGnssBaseline = new QSpinBox(moreBox);
    spGnssBaseline->setRange(0,1000);
    // 航向角
    QSpinBox* spHeading = new QSpinBox(moreBox);
    spHeading->setRange(0,360);
    // GNSS航向修正量
    QSpinBox* spGnssYawCorrection = new QSpinBox(moreBox);
    spGnssYawCorrection->setRange(-180,180);
    // GNSS俯仰修正量
    QSpinBox* spGnssPitchCorrection = new QSpinBox(moreBox);
    spGnssPitchCorrection->setRange(-5,5);

    moreGrid->addRow(tr("Install Error Correction"), spInstallError);
    moreGrid->addRow(tr("GNSS Combination Forbid"), spCombForbid);
    moreGrid->addRow(tr("Pitch Correction"), spPitch);
    moreGrid->addRow(tr("Roll Correction"), spRoll);
    moreGrid->addRow(tr("Yaw Correction"), spYaw);
    moreGrid->addRow(tr("GNSS East Velocity"), spGnssEast);
    moreGrid->addRow(tr("GNSS North Velocity"), spGnssNorth);
    moreGrid->addRow(tr("GNSS Up Velocity"), spGnssUp);
    moreGrid->addRow(tr("GNSS Longitude"), spGnssLon);
    moreGrid->addRow(tr("GNSS Latitude"), spGnssLat);
    moreGrid->addRow(tr("GNSS Altitude"), spGnssAlt);
    moreGrid->addRow(tr("UTC Year"), spUTCYear);
    moreGrid->addRow(tr("UTC Month"), spUTCMonth);
    moreGrid->addRow(tr("UTC Day"), spUTCDay);
    moreGrid->addRow(tr("UTC Hour"), spUTCHour);
    moreGrid->addRow(tr("UTC Minute"), spUTCMin);
    moreGrid->addRow(tr("UTC Second"), spUTCSec);
    moreGrid->addRow(tr("UTC Millisecond"), spUTCMsec);
    moreGrid->addRow(tr("GNSS PDOP"), spGnssPDOP);
    moreGrid->addRow(tr("GNSS Work State"), spGnssWork);
    moreGrid->addRow(tr("Atmosphere Height"), spAtmosphereHeight);
    moreGrid->addRow(tr("True Air Speed"), spTas);
    moreGrid->addRow(tr("GNSS Baseline Length"), spGnssBaseline);
    moreGrid->addRow(tr("Heading"), spHeading);
    moreGrid->addRow(tr("GNSS Yaw Correction"), spGnssYawCorrection);
    moreGrid->addRow(tr("GNSS Pitch Correction"), spGnssPitchCorrection);

    //moreLayout->addRow(cbPosVel);
    //moreLayout->addRow(cbUTC);

    main->addWidget(moreBox);

    // Interval and control buttons
    auto ctl = new QHBoxLayout();
    QLabel* lblInterval = new QLabel(tr("Interval (ms):"), w);
    QSpinBox* spInterval = new QSpinBox(w);
    spInterval->setRange(10, 60*60*1000);
    spInterval->setValue(1000);
    QPushButton* btnStart = new QPushButton(tr("Start"), w);
    QPushButton* btnStop = new QPushButton(tr("Stop"), w);
    QPushButton* btnSendOnce = new QPushButton(tr("Send Once"), w);
    ctl->addWidget(lblInterval);
    ctl->addWidget(spInterval);
    ctl->addWidget(btnStart);
    ctl->addWidget(btnStop);
    ctl->addWidget(btnSendOnce);
    main->addLayout(ctl);
    // 安装误差修正命令
    connect(spInstallError, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v){ m_installErrorCorrection = static_cast<uint8_t>(v); });
    // 航向角数据有效性
    connect(cbHeading, &QCheckBox::toggled, this, [this](bool v){ m_headingValid = v; OuterSend_HeadingValid = v; });
    // 装机基线长度有效标志
    connect(cbGnssBaselineValid, &QCheckBox::toggled, this, [this](bool v){ m_baselineLengthValid =v; });
    // GNSS数据包有效性
    connect(cbGnss, &QCheckBox::toggled, this, [this](bool v){ m_gnssDataValid = v; OuterSend_GNSSDataValid = v; });
    // 大气数据包有效性
    connect(cbAtm, &QCheckBox::toggled, this, [this](bool v){ m_atmosphereDataValid = v; OuterSend_AtmosphereDataValid = v; });
    // GNSS组合导航禁止命令
    connect(spCombForbid, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v){ m_gnssCombinationForbid = static_cast<uint8_t>(v); OuterSend_GNSSCombinationForbid = m_gnssCombinationForbid; });

    //俯仰 横滚 航向修正量
    connect(spPitch, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v){ m_PitchCorrection = static_cast<double>(v); });
    connect(spRoll, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v){ m_RollCorrection = static_cast<double>(v);  });
    connect(spYaw, qOverload<int>(&QSpinBox::valueChanged), this,   [this](int v){ m_YawCorrection = static_cast<double>(v);  });
    //GNSS 东北天速度，经纬，海拔高
    connect(spGnssEast, qOverload<int>(&QSpinBox::valueChanged), this, [](int v){ InnerGpsCache.velE = static_cast<double>(v); });
    connect(spGnssNorth, qOverload<int>(&QSpinBox::valueChanged), this, [](int v){ InnerGpsCache.velN = static_cast<double>(v); });
    connect(spGnssUp, qOverload<int>(&QSpinBox::valueChanged), this, [](int v){ InnerGpsCache.velD = static_cast<double>(v); });
    connect(spGnssLon, qOverload<int>(&QSpinBox::valueChanged), this, [](int v){ InnerGpsCache.lon = static_cast<double>(v) * M_PI / 180.0; });
    connect(spGnssLat, qOverload<int>(&QSpinBox::valueChanged), this, [](int v){ InnerGpsCache.lat = static_cast<double>(v) * M_PI / 180.0; });
    connect(spGnssAlt, qOverload<int>(&QSpinBox::valueChanged), this, [](int v){ InnerGpsCache.hMSL = static_cast<double>(v); });
    // UTC time: combine seconds + msec into InnerGpsCache.time.sec
    connect(spUTCYear, qOverload<int>(&QSpinBox::valueChanged), this, [](int v){ InnerGpsCache.time.year = v; });
    connect(spUTCMonth, qOverload<int>(&QSpinBox::valueChanged), this, [](int v){ InnerGpsCache.time.month = static_cast<uint8_t>(v); });
    connect(spUTCDay, qOverload<int>(&QSpinBox::valueChanged), this, [](int v){ InnerGpsCache.time.day = static_cast<uint8_t>(v); });
    connect(spUTCHour, qOverload<int>(&QSpinBox::valueChanged), this, [](int v){ InnerGpsCache.time.hour = static_cast<uint8_t>(v); });
    connect(spUTCMin, qOverload<int>(&QSpinBox::valueChanged), this, [](int v){ InnerGpsCache.time.min = static_cast<uint8_t>(v); });
    connect(spUTCSec, qOverload<int>(&QSpinBox::valueChanged), this, [spUTCMsec](int sec){InnerGpsCache.time.sec = static_cast<float>(sec) ; });
    connect(spUTCMsec, qOverload<int>(&QSpinBox::valueChanged), this, [spUTCSec](int msec){InnerGpsCache.time.msec = static_cast<float>(msec); });
    // GNSS_PDOP
    connect(spGnssPDOP, qOverload<int>(&QSpinBox::valueChanged), this, [](int v){ InnerGpsCache.pDOP = static_cast<double>(v); });
    // GNSS工作状态
    connect(spGnssWork, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v){ m_gnssWorkState = static_cast<uint8_t>(v); OuterSend_GNSSWorkState = m_gnssWorkState; });
    // 位置速度数据有效
    connect(cbPosVel, &QCheckBox::toggled, this, [this](bool v){ m_positionVelocityValid = v; OuterSend_PositionVelocityValid = v; });
    // UTC时间数据有效
    connect(cbUTC, &QCheckBox::toggled, this, [this](bool v){ m_utcTimeValid = v; OuterSend_UTCTimeValid = v; });

    connect(spAtmosphereHeight, qOverload<int>(&QSpinBox::valueChanged), this, [](int v){ InnerGpsCache.baroAlt = static_cast<double>(v); });
    connect(spTas, qOverload<int>(&QSpinBox::valueChanged), this, [](int v){ InnerGpsCache.trueAirSpeed = static_cast<double>(v); });
    connect(spGnssBaseline, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v){ m_baselineLength = v; });
    connect(spHeading, qOverload<int>(&QSpinBox::valueChanged), this, [](int v){ InnerGpsCache.heading = static_cast<double>(v) * M_PI / 180.0; });

    // cbAtmosphereHeight Valid
    connect(cbAtmosphereHeight, &QCheckBox::toggled, this, [this](bool v){ m_AtmosphereHeightDataValid = v; OuterSend_AtmosphereHeightDataValid = v; });
    // cbTas Valid
    connect(cbTas, &QCheckBox::toggled, this, [this](bool v){ m_TrueAirSpeedDataValid = v; OuterSend_TrueAirSpeedDataValid = v; });
    // GNSS航向修正量    GNSS俯仰修正量
    connect(spGnssYawCorrection, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v){ m_GnssYawCorrection = static_cast<double>(v);   });
    connect(spGnssPitchCorrection, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v){ m_GnssPitchCorrection = static_cast<double>(v);   });

    /////////////////////////////////////
    connect(spInterval, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v){ m_intervalMs = v; if (m_timer.isActive()) m_timer.start(m_intervalMs); });
    connect(btnStart, &QPushButton::clicked, this, &AutoNavPanel::onStartClicked);
    connect(btnStop, &QPushButton::clicked, this, &AutoNavPanel::onStopClicked);
    connect(btnSendOnce, &QPushButton::clicked, this, &AutoNavPanel::onSendOnceClicked);

    return w;
}

void AutoNavPanel::onStartClicked()
{
    if (m_intervalMs <= 0) m_intervalMs = 1000;
    m_timer.start(m_intervalMs);
}

void AutoNavPanel::onStopClicked()
{
    m_timer.stop();
}

void AutoNavPanel::onSendOnceClicked()
{
    assembleAndSendFrame();
}

void AutoNavPanel::onTimerTimeout()
{
    assembleAndSendFrame();
}

static inline uint8_t byteOfInt16(int16_t v, int idx)
{
    return static_cast<uint8_t>((v >> (8*idx)) & 0xFF);
}

static inline uint8_t byteOfInt32(int32_t v, int idx)
{
    return static_cast<uint8_t>((v >> (8*idx)) & 0xFF);
}

void AutoNavPanel::assembleAndSendFrame()
{
    // Mirror the original SendFMC_AutoNavCommand behavior
    uint8_t dataSend[200];
    int byteNum = 0;

    dataSend[byteNum++] = 0xAA;
    dataSend[byteNum++] = 0x55;
    dataSend[byteNum++] = 0xB1;
    dataSend[byteNum++] = 0x38; // length (56)
    dataSend[byteNum++] = m_frameCnt++;

    uint8_t sys_cmd = 0;
    sys_cmd |= ((m_installErrorCorrection & 0x3) << 0);
    sys_cmd |= ((m_headingValid & 0x1) << 2);
    sys_cmd |= ((m_baselineLengthValid & 0x1) << 3);
    sys_cmd |= ((m_gnssDataValid & 0x1) << 4);
    sys_cmd |= ((m_atmosphereDataValid & 0x1) << 5);
    sys_cmd |= ((m_gnssCombinationForbid & 0x3) << 6);
    dataSend[byteNum++] = sys_cmd;

    // pitch correction (7-8) -> use 0
    int16_t temp_int16 = m_PitchCorrection / 0.0054931640625;
    dataSend[byteNum++] = byteOfInt16(temp_int16, 0);
    dataSend[byteNum++] = byteOfInt16(temp_int16, 1);

    // roll correction (9-10)
    temp_int16 = m_RollCorrection / 0.0054931640625;
    dataSend[byteNum++] = byteOfInt16(temp_int16, 0);
    dataSend[byteNum++] = byteOfInt16(temp_int16, 1);

    // heading correction (11-12)
    temp_int16 = m_YawCorrection / 0.0054931640625;
    dataSend[byteNum++] = byteOfInt16(temp_int16, 0);
    dataSend[byteNum++] = byteOfInt16(temp_int16, 1);

    // velocities: East, North, Down (scaled by 100)
    int16_t v16 = static_cast<int16_t>(std::round(InnerGpsCache.velE * 100.0));
    if (v16 > 32700) v16 = 32700; if (v16 < -32700) v16 = -32700;
    dataSend[byteNum++] = byteOfInt16(v16,0);
    dataSend[byteNum++] = byteOfInt16(v16,1);

    v16 = static_cast<int16_t>(std::round(InnerGpsCache.velN * 100.0));
    if (v16 > 32700) v16 = 32700; if (v16 < -32700) v16 = -32700;
    dataSend[byteNum++] = byteOfInt16(v16,0);
    dataSend[byteNum++] = byteOfInt16(v16,1);

    v16 = static_cast<int16_t>(std::round(InnerGpsCache.velD * 100.0));
    if (v16 > 32700) v16 = 32700; if (v16 < -32700) v16 = -32700;
    dataSend[byteNum++] = byteOfInt16(v16,0);
    dataSend[byteNum++] = byteOfInt16(v16,1);

    // GNSS lon/lat scaled per original: (lon * RAD2DEG * 11930464.7056)
    int32_t temp_int32 = static_cast<int32_t>(InnerGpsCache.lon * RAD2DEG * 11930464.7056);
    dataSend[byteNum++] = byteOfInt32(temp_int32,0);
    dataSend[byteNum++] = byteOfInt32(temp_int32,1);
    dataSend[byteNum++] = byteOfInt32(temp_int32,2);
    dataSend[byteNum++] = byteOfInt32(temp_int32,3);

    temp_int32 = static_cast<int32_t>(InnerGpsCache.lat * RAD2DEG * 11930464.7056);
    dataSend[byteNum++] = byteOfInt32(temp_int32,0);
    dataSend[byteNum++] = byteOfInt32(temp_int32,1);
    dataSend[byteNum++] = byteOfInt32(temp_int32,2);
    dataSend[byteNum++] = byteOfInt32(temp_int32,3);

    temp_int32 = static_cast<int32_t>(InnerGpsCache.hMSL * 100.0);
    dataSend[byteNum++] = byteOfInt32(temp_int32,0);
    dataSend[byteNum++] = byteOfInt32(temp_int32,1);
    dataSend[byteNum++] = byteOfInt32(temp_int32,2);
    dataSend[byteNum++] = byteOfInt32(temp_int32,3);

    // UTC time year (16-bit little endian)
    uint16_t utc_year = static_cast<uint16_t>(InnerGpsCache.time.year);
    dataSend[byteNum++] = utc_year & 0xFF;
    dataSend[byteNum++] = (utc_year >> 8) & 0xFF;
    // month, day, hour, min, sec
    dataSend[byteNum++] = InnerGpsCache.time.month;
    dataSend[byteNum++] = InnerGpsCache.time.day;
    dataSend[byteNum++] = InnerGpsCache.time.hour;
    dataSend[byteNum++] = InnerGpsCache.time.min;
    dataSend[byteNum++] = static_cast<uint8_t>(InnerGpsCache.time.sec);
    dataSend[byteNum++] = static_cast<uint8_t>(InnerGpsCache.time.msec/10.0);
    // PDOP
    dataSend[byteNum++] = static_cast<uint8_t>(InnerGpsCache.pDOP * 10.0);

    // GNSS status
    uint8_t gnssWorkStatus = 0;
    gnssWorkStatus = (m_gnssWorkState & 0x3) | ((m_positionVelocityValid & 0x1) << 2) | ((m_utcTimeValid & 0x1) << 3);
    dataSend[byteNum++] = gnssWorkStatus;

    // barometric altitude (41-42) - use 0 as in original
    temp_int16 = static_cast<int16_t>(InnerGpsCache.baroAlt*2.0);
    dataSend[byteNum++] = byteOfInt16(temp_int16,0);
    dataSend[byteNum++] = byteOfInt16(temp_int16,1);

    // reserved 43-44
    dataSend[byteNum++] = 0x00;
    dataSend[byteNum++] = 0x00;

    // true airspeed 45-46 - use 0
    uint16_t trueAirSpeed = static_cast<int16_t>(InnerGpsCache.trueAirSpeed*128.0);
    dataSend[byteNum++] = (trueAirSpeed & 0xFF);
    dataSend[byteNum++] = ((trueAirSpeed >> 8) & 0xFF);

    uint16_t baselineLength = static_cast<int16_t>(m_baselineLength * 10.0);
    dataSend[byteNum++] = (baselineLength & 0xFF);
    // reserved 48
    dataSend[byteNum++] = 0x00;

    // heading 49-50 (LSB=0.0054931640625 deg)
    double limitedValue = InnerGpsCache.heading * RAD2DEG;
    while (limitedValue >= 360.0) limitedValue -= 360.0;
    while (limitedValue < 0.0) limitedValue += 360.0;
    uint16_t heading = static_cast<uint16_t>(limitedValue / 0.0054931640625);
    dataSend[byteNum++] = heading & 0xFF;
    dataSend[byteNum++] = (heading >> 8) & 0xFF;

    // atmosphere valid byte
    uint8_t atmValid = 0;
    atmValid |= (m_AtmosphereHeightDataValid & 0x1);
    atmValid |= ((m_TrueAirSpeedDataValid & 0x1) << 2);
    dataSend[byteNum++] = atmValid;

    // GNSS heading correction 52-53
    temp_int16 = static_cast<int16_t>(m_GnssYawCorrection/0.0054931640625);
    dataSend[byteNum++] = byteOfInt16(temp_int16,0);
    dataSend[byteNum++] = byteOfInt16(temp_int16,1);

    // GNSS pitch correction 54-55
    temp_int16 = static_cast<int16_t>(m_GnssPitchCorrection/0.0054931640625);
    dataSend[byteNum++] = byteOfInt16(temp_int16,0);
    dataSend[byteNum++] = byteOfInt16(temp_int16,1);

    // checksum (from byte 2 to byteNum-1)
    uint8_t checkBytes = 0;
    for (int i = 2; i < byteNum; ++i) checkBytes += dataSend[i];
    dataSend[byteNum++] = checkBytes & 0xFF;

    if (m_port && m_port->isOpen()) {
        QByteArray out(reinterpret_cast<const char*>(dataSend), byteNum);
        m_port->write(out);
        m_port->flush();
    }
}
