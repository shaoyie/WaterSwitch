/********************************************************************************
** Form generated from reading UI file 'wsremotecontrol.ui'
**
** Created by: Qt User Interface Compiler version 5.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WSREMOTECONTROL_H
#define UI_WSREMOTECONTROL_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLCDNumber>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_WsRemoteControl
{
public:
    QVBoxLayout *verticalLayout;
    QTabWidget *tabWidget;
    QWidget *workTab;
    QVBoxLayout *verticalLayout_8;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_6;
    QLabel *label;
    QLabel *workModeLabel;
    QSpacerItem *horizontalSpacer_5;
    QHBoxLayout *horizontalLayout_10;
    QVBoxLayout *verticalLayout_6;
    QSpacerItem *verticalSpacer_4;
    QLCDNumber *envTemp;
    QSpacerItem *verticalSpacer_5;
    QVBoxLayout *verticalLayout_4;
    QLabel *fireIcon;
    QLabel *fireSelector;
    QSpacerItem *horizontalSpacer_4;
    QVBoxLayout *verticalLayout_7;
    QSpacerItem *verticalSpacer_6;
    QLCDNumber *salorTemp;
    QSpacerItem *verticalSpacer_7;
    QVBoxLayout *verticalLayout_3;
    QLabel *salorIcon;
    QHBoxLayout *horizontalLayout_9;
    QSpacerItem *horizontalSpacer_2;
    QLabel *upWaterIcon;
    QLabel *downWaterIcon;
    QSpacerItem *horizontalSpacer_3;
    QLabel *salorSelector;
    QVBoxLayout *verticalLayout_5;
    QSpacerItem *verticalSpacer;
    QLabel *tempAlarm;
    QSpacerItem *verticalSpacer_2;
    QLabel *pumpAlarm;
    QSpacerItem *verticalSpacer_3;
    QHBoxLayout *horizontalLayout_11;
    QPushButton *workModeButton;
    QPushButton *heaterButton;
    QWidget *setupTab;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_2;
    QLabel *caliLabel;
    QScrollBar *caliScrollBar;
    QHBoxLayout *horizontalLayout_5;
    QLabel *label_3;
    QLabel *winterThresholdLabel;
    QScrollBar *winterThresholdScrollBar;
    QHBoxLayout *horizontalLayout_6;
    QLabel *label_4;
    QLabel *winterSwitchTempLabel;
    QScrollBar *winterSwitchTempScrollBar;
    QHBoxLayout *horizontalLayout_7;
    QLabel *label_5;
    QLabel *summerSwitchTempLabel;
    QScrollBar *summerSwitchTempScrollBar;
    QHBoxLayout *horizontalLayout_8;
    QPushButton *saveButton;
    QPushButton *resetButton;
    QWidget *configTab;
    QVBoxLayout *verticalLayout_9;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_6;
    QComboBox *uart;
    QSpacerItem *horizontalSpacer;
    QHBoxLayout *horizontalLayout_3;
    QCheckBox *allCB;
    QCheckBox *infoCB;
    QCheckBox *dbgCB;
    QCheckBox *errCB;
    QCheckBox *outputCB;
    QTextBrowser *logBrowser;

    void setupUi(QWidget *WsRemoteControl)
    {
        if (WsRemoteControl->objectName().isEmpty())
            WsRemoteControl->setObjectName(QStringLiteral("WsRemoteControl"));
        WsRemoteControl->resize(340, 230);
        verticalLayout = new QVBoxLayout(WsRemoteControl);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        tabWidget = new QTabWidget(WsRemoteControl);
        tabWidget->setObjectName(QStringLiteral("tabWidget"));
        workTab = new QWidget();
        workTab->setObjectName(QStringLiteral("workTab"));
        verticalLayout_8 = new QVBoxLayout(workTab);
        verticalLayout_8->setSpacing(6);
        verticalLayout_8->setContentsMargins(11, 11, 11, 11);
        verticalLayout_8->setObjectName(QStringLiteral("verticalLayout_8"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalSpacer_6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_6);

        label = new QLabel(workTab);
        label->setObjectName(QStringLiteral("label"));

        horizontalLayout->addWidget(label);

        workModeLabel = new QLabel(workTab);
        workModeLabel->setObjectName(QStringLiteral("workModeLabel"));
        QFont font;
        font.setFamily(QStringLiteral("Arial"));
        font.setPointSize(11);
        font.setBold(true);
        font.setWeight(75);
        workModeLabel->setFont(font);
        workModeLabel->setTextFormat(Qt::AutoText);
        workModeLabel->setScaledContents(false);

        horizontalLayout->addWidget(workModeLabel);

        horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_5);


        verticalLayout_8->addLayout(horizontalLayout);

        horizontalLayout_10 = new QHBoxLayout();
        horizontalLayout_10->setSpacing(6);
        horizontalLayout_10->setObjectName(QStringLiteral("horizontalLayout_10"));
        verticalLayout_6 = new QVBoxLayout();
        verticalLayout_6->setSpacing(6);
        verticalLayout_6->setObjectName(QStringLiteral("verticalLayout_6"));
        verticalSpacer_4 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_6->addItem(verticalSpacer_4);

        envTemp = new QLCDNumber(workTab);
        envTemp->setObjectName(QStringLiteral("envTemp"));
        envTemp->setAutoFillBackground(false);
        envTemp->setFrameShape(QFrame::StyledPanel);
        envTemp->setDigitCount(2);
        envTemp->setSegmentStyle(QLCDNumber::Flat);
        envTemp->setProperty("intValue", QVariant(-1));

        verticalLayout_6->addWidget(envTemp);

        verticalSpacer_5 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_6->addItem(verticalSpacer_5);


        horizontalLayout_10->addLayout(verticalLayout_6);

        verticalLayout_4 = new QVBoxLayout();
        verticalLayout_4->setSpacing(6);
        verticalLayout_4->setObjectName(QStringLiteral("verticalLayout_4"));
        fireIcon = new QLabel(workTab);
        fireIcon->setObjectName(QStringLiteral("fireIcon"));
        fireIcon->setPixmap(QPixmap(QString::fromUtf8(":/icons/fire_g.png")));
        fireIcon->setScaledContents(false);
        fireIcon->setAlignment(Qt::AlignCenter);

        verticalLayout_4->addWidget(fireIcon);

        fireSelector = new QLabel(workTab);
        fireSelector->setObjectName(QStringLiteral("fireSelector"));
        fireSelector->setPixmap(QPixmap(QString::fromUtf8(":/icons/selector.png")));
        fireSelector->setScaledContents(false);
        fireSelector->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);

        verticalLayout_4->addWidget(fireSelector);


        horizontalLayout_10->addLayout(verticalLayout_4);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_10->addItem(horizontalSpacer_4);

        verticalLayout_7 = new QVBoxLayout();
        verticalLayout_7->setSpacing(6);
        verticalLayout_7->setObjectName(QStringLiteral("verticalLayout_7"));
        verticalSpacer_6 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_7->addItem(verticalSpacer_6);

        salorTemp = new QLCDNumber(workTab);
        salorTemp->setObjectName(QStringLiteral("salorTemp"));
        salorTemp->setFrameShape(QFrame::StyledPanel);
        salorTemp->setSmallDecimalPoint(false);
        salorTemp->setDigitCount(2);
        salorTemp->setSegmentStyle(QLCDNumber::Flat);
        salorTemp->setProperty("intValue", QVariant(-1));

        verticalLayout_7->addWidget(salorTemp);

        verticalSpacer_7 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_7->addItem(verticalSpacer_7);


        horizontalLayout_10->addLayout(verticalLayout_7);

        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setSpacing(6);
        verticalLayout_3->setObjectName(QStringLiteral("verticalLayout_3"));
        salorIcon = new QLabel(workTab);
        salorIcon->setObjectName(QStringLiteral("salorIcon"));
        salorIcon->setPixmap(QPixmap(QString::fromUtf8(":/icons/salor4.png")));
        salorIcon->setScaledContents(false);
        salorIcon->setAlignment(Qt::AlignHCenter|Qt::AlignTop);

        verticalLayout_3->addWidget(salorIcon);

        horizontalLayout_9 = new QHBoxLayout();
        horizontalLayout_9->setSpacing(6);
        horizontalLayout_9->setObjectName(QStringLiteral("horizontalLayout_9"));
        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_9->addItem(horizontalSpacer_2);

        upWaterIcon = new QLabel(workTab);
        upWaterIcon->setObjectName(QStringLiteral("upWaterIcon"));
        upWaterIcon->setMaximumSize(QSize(16, 21));
        upWaterIcon->setPixmap(QPixmap(QString::fromUtf8(":/icons/up_no.png")));
        upWaterIcon->setScaledContents(true);

        horizontalLayout_9->addWidget(upWaterIcon);

        downWaterIcon = new QLabel(workTab);
        downWaterIcon->setObjectName(QStringLiteral("downWaterIcon"));
        downWaterIcon->setMaximumSize(QSize(16, 21));
        downWaterIcon->setPixmap(QPixmap(QString::fromUtf8(":/icons/down_no.png")));
        downWaterIcon->setScaledContents(true);

        horizontalLayout_9->addWidget(downWaterIcon);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_9->addItem(horizontalSpacer_3);


        verticalLayout_3->addLayout(horizontalLayout_9);

        salorSelector = new QLabel(workTab);
        salorSelector->setObjectName(QStringLiteral("salorSelector"));
        salorSelector->setPixmap(QPixmap(QString::fromUtf8(":/icons/selector.png")));
        salorSelector->setScaledContents(false);
        salorSelector->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);

        verticalLayout_3->addWidget(salorSelector);


        horizontalLayout_10->addLayout(verticalLayout_3);

        verticalLayout_5 = new QVBoxLayout();
        verticalLayout_5->setSpacing(6);
        verticalLayout_5->setObjectName(QStringLiteral("verticalLayout_5"));
        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_5->addItem(verticalSpacer);

        tempAlarm = new QLabel(workTab);
        tempAlarm->setObjectName(QStringLiteral("tempAlarm"));
        tempAlarm->setEnabled(true);
        tempAlarm->setPixmap(QPixmap(QString::fromUtf8(":/icons/alarm.png")));
        tempAlarm->setScaledContents(false);

        verticalLayout_5->addWidget(tempAlarm);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_5->addItem(verticalSpacer_2);

        pumpAlarm = new QLabel(workTab);
        pumpAlarm->setObjectName(QStringLiteral("pumpAlarm"));
        pumpAlarm->setPixmap(QPixmap(QString::fromUtf8(":/icons/alarm.png")));
        pumpAlarm->setScaledContents(false);

        verticalLayout_5->addWidget(pumpAlarm);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_5->addItem(verticalSpacer_3);


        horizontalLayout_10->addLayout(verticalLayout_5);


        verticalLayout_8->addLayout(horizontalLayout_10);

        horizontalLayout_11 = new QHBoxLayout();
        horizontalLayout_11->setSpacing(6);
        horizontalLayout_11->setObjectName(QStringLiteral("horizontalLayout_11"));
        workModeButton = new QPushButton(workTab);
        workModeButton->setObjectName(QStringLiteral("workModeButton"));

        horizontalLayout_11->addWidget(workModeButton);

        heaterButton = new QPushButton(workTab);
        heaterButton->setObjectName(QStringLiteral("heaterButton"));

        horizontalLayout_11->addWidget(heaterButton);


        verticalLayout_8->addLayout(horizontalLayout_11);

        tabWidget->addTab(workTab, QString());
        setupTab = new QWidget();
        setupTab->setObjectName(QStringLiteral("setupTab"));
        verticalLayout_2 = new QVBoxLayout(setupTab);
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setContentsMargins(11, 11, 11, 11);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setSpacing(6);
        horizontalLayout_4->setObjectName(QStringLiteral("horizontalLayout_4"));
        label_2 = new QLabel(setupTab);
        label_2->setObjectName(QStringLiteral("label_2"));

        horizontalLayout_4->addWidget(label_2);

        caliLabel = new QLabel(setupTab);
        caliLabel->setObjectName(QStringLiteral("caliLabel"));

        horizontalLayout_4->addWidget(caliLabel);

        caliScrollBar = new QScrollBar(setupTab);
        caliScrollBar->setObjectName(QStringLiteral("caliScrollBar"));
        caliScrollBar->setMinimum(200);
        caliScrollBar->setMaximum(400);
        caliScrollBar->setValue(293);
        caliScrollBar->setOrientation(Qt::Horizontal);

        horizontalLayout_4->addWidget(caliScrollBar);

        horizontalLayout_4->setStretch(0, 4);
        horizontalLayout_4->setStretch(1, 1);
        horizontalLayout_4->setStretch(2, 5);

        verticalLayout_2->addLayout(horizontalLayout_4);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setSpacing(6);
        horizontalLayout_5->setObjectName(QStringLiteral("horizontalLayout_5"));
        label_3 = new QLabel(setupTab);
        label_3->setObjectName(QStringLiteral("label_3"));

        horizontalLayout_5->addWidget(label_3);

        winterThresholdLabel = new QLabel(setupTab);
        winterThresholdLabel->setObjectName(QStringLiteral("winterThresholdLabel"));

        horizontalLayout_5->addWidget(winterThresholdLabel);

        winterThresholdScrollBar = new QScrollBar(setupTab);
        winterThresholdScrollBar->setObjectName(QStringLiteral("winterThresholdScrollBar"));
        winterThresholdScrollBar->setOrientation(Qt::Horizontal);

        horizontalLayout_5->addWidget(winterThresholdScrollBar);

        horizontalLayout_5->setStretch(0, 4);
        horizontalLayout_5->setStretch(1, 1);
        horizontalLayout_5->setStretch(2, 5);

        verticalLayout_2->addLayout(horizontalLayout_5);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setSpacing(6);
        horizontalLayout_6->setObjectName(QStringLiteral("horizontalLayout_6"));
        label_4 = new QLabel(setupTab);
        label_4->setObjectName(QStringLiteral("label_4"));

        horizontalLayout_6->addWidget(label_4);

        winterSwitchTempLabel = new QLabel(setupTab);
        winterSwitchTempLabel->setObjectName(QStringLiteral("winterSwitchTempLabel"));

        horizontalLayout_6->addWidget(winterSwitchTempLabel);

        winterSwitchTempScrollBar = new QScrollBar(setupTab);
        winterSwitchTempScrollBar->setObjectName(QStringLiteral("winterSwitchTempScrollBar"));
        winterSwitchTempScrollBar->setOrientation(Qt::Horizontal);

        horizontalLayout_6->addWidget(winterSwitchTempScrollBar);

        horizontalLayout_6->setStretch(0, 4);
        horizontalLayout_6->setStretch(1, 1);
        horizontalLayout_6->setStretch(2, 5);

        verticalLayout_2->addLayout(horizontalLayout_6);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setSpacing(6);
        horizontalLayout_7->setObjectName(QStringLiteral("horizontalLayout_7"));
        label_5 = new QLabel(setupTab);
        label_5->setObjectName(QStringLiteral("label_5"));

        horizontalLayout_7->addWidget(label_5);

        summerSwitchTempLabel = new QLabel(setupTab);
        summerSwitchTempLabel->setObjectName(QStringLiteral("summerSwitchTempLabel"));

        horizontalLayout_7->addWidget(summerSwitchTempLabel);

        summerSwitchTempScrollBar = new QScrollBar(setupTab);
        summerSwitchTempScrollBar->setObjectName(QStringLiteral("summerSwitchTempScrollBar"));
        summerSwitchTempScrollBar->setOrientation(Qt::Horizontal);

        horizontalLayout_7->addWidget(summerSwitchTempScrollBar);

        horizontalLayout_7->setStretch(0, 4);
        horizontalLayout_7->setStretch(1, 1);
        horizontalLayout_7->setStretch(2, 5);

        verticalLayout_2->addLayout(horizontalLayout_7);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setSpacing(6);
        horizontalLayout_8->setObjectName(QStringLiteral("horizontalLayout_8"));
        saveButton = new QPushButton(setupTab);
        saveButton->setObjectName(QStringLiteral("saveButton"));

        horizontalLayout_8->addWidget(saveButton);

        resetButton = new QPushButton(setupTab);
        resetButton->setObjectName(QStringLiteral("resetButton"));

        horizontalLayout_8->addWidget(resetButton);


        verticalLayout_2->addLayout(horizontalLayout_8);

        tabWidget->addTab(setupTab, QString());
        configTab = new QWidget();
        configTab->setObjectName(QStringLiteral("configTab"));
        verticalLayout_9 = new QVBoxLayout(configTab);
        verticalLayout_9->setSpacing(6);
        verticalLayout_9->setContentsMargins(11, 11, 11, 11);
        verticalLayout_9->setObjectName(QStringLiteral("verticalLayout_9"));
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        label_6 = new QLabel(configTab);
        label_6->setObjectName(QStringLiteral("label_6"));

        horizontalLayout_2->addWidget(label_6);

        uart = new QComboBox(configTab);
        uart->setObjectName(QStringLiteral("uart"));

        horizontalLayout_2->addWidget(uart);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);


        verticalLayout_9->addLayout(horizontalLayout_2);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        allCB = new QCheckBox(configTab);
        allCB->setObjectName(QStringLiteral("allCB"));

        horizontalLayout_3->addWidget(allCB);

        infoCB = new QCheckBox(configTab);
        infoCB->setObjectName(QStringLiteral("infoCB"));

        horizontalLayout_3->addWidget(infoCB);

        dbgCB = new QCheckBox(configTab);
        dbgCB->setObjectName(QStringLiteral("dbgCB"));

        horizontalLayout_3->addWidget(dbgCB);

        errCB = new QCheckBox(configTab);
        errCB->setObjectName(QStringLiteral("errCB"));

        horizontalLayout_3->addWidget(errCB);

        outputCB = new QCheckBox(configTab);
        outputCB->setObjectName(QStringLiteral("outputCB"));

        horizontalLayout_3->addWidget(outputCB);


        verticalLayout_9->addLayout(horizontalLayout_3);

        logBrowser = new QTextBrowser(configTab);
        logBrowser->setObjectName(QStringLiteral("logBrowser"));

        verticalLayout_9->addWidget(logBrowser);

        tabWidget->addTab(configTab, QString());

        verticalLayout->addWidget(tabWidget);


        retranslateUi(WsRemoteControl);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(WsRemoteControl);
    } // setupUi

    void retranslateUi(QWidget *WsRemoteControl)
    {
        WsRemoteControl->setWindowTitle(QApplication::translate("WsRemoteControl", "WsRemoteControl", Q_NULLPTR));
        label->setText(QApplication::translate("WsRemoteControl", "Work mode: ", Q_NULLPTR));
        workModeLabel->setText(QApplication::translate("WsRemoteControl", "Auto", Q_NULLPTR));
        fireIcon->setText(QString());
        fireSelector->setText(QString());
        salorIcon->setText(QString());
        upWaterIcon->setText(QString());
        downWaterIcon->setText(QString());
        salorSelector->setText(QString());
        tempAlarm->setText(QString());
        pumpAlarm->setText(QString());
        workModeButton->setText(QApplication::translate("WsRemoteControl", "Work mode", Q_NULLPTR));
        heaterButton->setText(QApplication::translate("WsRemoteControl", "Salor/Fire", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(workTab), QApplication::translate("WsRemoteControl", "Status", Q_NULLPTR));
        label_2->setText(QApplication::translate("WsRemoteControl", "Env temp. calibrate", Q_NULLPTR));
        caliLabel->setText(QString());
        label_3->setText(QApplication::translate("WsRemoteControl", "Winter thershold", Q_NULLPTR));
        winterThresholdLabel->setText(QString());
        label_4->setText(QApplication::translate("WsRemoteControl", "Winter switch temp", Q_NULLPTR));
        winterSwitchTempLabel->setText(QString());
        label_5->setText(QApplication::translate("WsRemoteControl", "Summer switch temp", Q_NULLPTR));
        summerSwitchTempLabel->setText(QString());
        saveButton->setText(QApplication::translate("WsRemoteControl", "Save", Q_NULLPTR));
        resetButton->setText(QApplication::translate("WsRemoteControl", "Reset", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(setupTab), QApplication::translate("WsRemoteControl", "Setup", Q_NULLPTR));
#ifndef QT_NO_WHATSTHIS
        configTab->setWhatsThis(QApplication::translate("WsRemoteControl", "<html><head/><body><p><br/></p></body></html>", Q_NULLPTR));
#endif // QT_NO_WHATSTHIS
        label_6->setText(QApplication::translate("WsRemoteControl", "Serial port", Q_NULLPTR));
        allCB->setText(QApplication::translate("WsRemoteControl", "All", Q_NULLPTR));
        infoCB->setText(QApplication::translate("WsRemoteControl", "Info", Q_NULLPTR));
        dbgCB->setText(QApplication::translate("WsRemoteControl", "Debug", Q_NULLPTR));
        errCB->setText(QApplication::translate("WsRemoteControl", "Error", Q_NULLPTR));
        outputCB->setText(QApplication::translate("WsRemoteControl", "Output", Q_NULLPTR));
        tabWidget->setTabText(tabWidget->indexOf(configTab), QApplication::translate("WsRemoteControl", "Config", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class WsRemoteControl: public Ui_WsRemoteControl {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WSREMOTECONTROL_H
