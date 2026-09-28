/* SPDX-FileCopyrightText: 2026 tomek7667 <git@cyber-man.pl>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "AudioDeviceModel.h"
#include "settings.h"

#include <KLocalizedString>

#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

using namespace Qt::StringLiterals;

class AudioDeviceModelTest : public QObject
{
    Q_OBJECT

private:
    static int rowOf(const AudioDeviceModel &model, const QString &nodeName)
    {
        for (int row = 2; row < model.rowCount(); ++row) {
            if (model.index(row).data(AudioDeviceModel::NodeNameRole).toString() == nodeName) {
                return row;
            }
        }
        return -1;
    }

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        KLocalizedString::setApplicationDomain(QByteArrayLiteral("spectacle"));
    }

    void init()
    {
        AudioDeviceModel::Selection::defaults().saveToSettings();
    }

    void testDefaultRows()
    {
        AudioDeviceModel model;
        QVERIFY(model.rowCount() >= 2);

        const auto output = model.index(0);
        QCOMPARE(output.data(AudioDeviceModel::DeviceTypeRole).toInt(), AudioDeviceMonitor::Output);
        QVERIFY(output.data(AudioDeviceModel::NodeNameRole).toString().isEmpty());
        QCOMPARE(output.data(Qt::CheckStateRole).toInt(), Qt::Unchecked);

        const auto input = model.index(1);
        QCOMPARE(input.data(AudioDeviceModel::DeviceTypeRole).toInt(), AudioDeviceMonitor::Input);
        QVERIFY(input.data(AudioDeviceModel::NodeNameRole).toString().isEmpty());

        // Nothing selected means the recording has no sound
        QVERIFY(!model.hasSelection());
        QVERIFY(model.selection().isEmpty());
        QCOMPARE(model.iconName(), u"audio-volume-muted"_s);
    }

    void testToggleDefaultDevice()
    {
        AudioDeviceModel model;
        QSignalSpy selectionSpy(&model, &AudioDeviceModel::selectionChanged);

        QVERIFY(model.setData(model.index(1), Qt::Checked, Qt::CheckStateRole));
        QCOMPARE(selectionSpy.count(), 1);
        QVERIFY(model.selection().defaultInput);
        QVERIFY(!model.selection().defaultOutput);
        QCOMPARE(model.index(1).data(Qt::CheckStateRole).toInt(), Qt::Checked);
        QCOMPARE(model.summary(), model.index(1).data().toString());
        QCOMPARE(model.iconName(), u"audio-volume-high"_s);

        // Setting the same state again is not a change
        QVERIFY(!model.setData(model.index(1), Qt::Checked, Qt::CheckStateRole));
        QCOMPARE(selectionSpy.count(), 1);

        QVERIFY(model.setData(model.index(1), Qt::Unchecked, Qt::CheckStateRole));
        QVERIFY(model.selection().isEmpty());
    }

    void testMissingDevicesAreListed()
    {
        AudioDeviceModel model;
        AudioDeviceModel::Selection selection;
        selection.outputs = {u"spectacle-test-missing-sink"_s};
        selection.inputs = {u"spectacle-test-missing-source"_s};
        model.setSelection(selection);

        const int sinkRow = rowOf(model, u"spectacle-test-missing-sink"_s);
        const int sourceRow = rowOf(model, u"spectacle-test-missing-source"_s);
        QVERIFY(sinkRow >= 2);
        QVERIFY(sourceRow > sinkRow);

        const auto sink = model.index(sinkRow);
        QCOMPARE(sink.data(AudioDeviceModel::DeviceTypeRole).toInt(), AudioDeviceMonitor::Output);
        QCOMPARE(sink.data(AudioDeviceModel::ConnectedRole).toBool(), false);
        QCOMPARE(sink.data(Qt::CheckStateRole).toInt(), Qt::Checked);
        QVERIFY(sink.data().toString().contains(u"spectacle-test-missing-sink"_s));
        QCOMPARE(model.index(sourceRow).data(AudioDeviceModel::DeviceTypeRole).toInt(), AudioDeviceMonitor::Input);

        QCOMPARE(model.summary(), u"2 devices"_s);

        // Deselecting keeps the row until the list is rebuilt
        const int rowCount = model.rowCount();
        QVERIFY(model.setData(sink, Qt::Unchecked, Qt::CheckStateRole));
        QCOMPARE(model.rowCount(), rowCount);
        QVERIFY(model.selection().outputs.isEmpty());
        QCOMPARE(model.selection().inputs, selection.inputs);

        model.setSelection({});
        QCOMPARE(rowOf(model, u"spectacle-test-missing-sink"_s), -1);
        QCOMPARE(rowOf(model, u"spectacle-test-missing-source"_s), -1);
    }

    void testSharedInstanceFollowsSettings()
    {
        auto model = AudioDeviceModel::instance();
        QVERIFY(model->selection().isEmpty());

        QVERIFY(model->setData(model->index(0), Qt::Checked, Qt::CheckStateRole));
        QVERIFY(Settings::videoRecordSystemAudio());
        QVERIFY(!Settings::videoRecordMicrophone());

        Settings::setVideoRecordedAudioSources({u"spectacle-test-source"_s});
        QCOMPARE(model->selection().inputs, QStringList{u"spectacle-test-source"_s});
        QVERIFY(model->selection().defaultOutput);
        QVERIFY(rowOf(*model, u"spectacle-test-source"_s) >= 2);

        const int row = rowOf(*model, u"spectacle-test-source"_s);
        QVERIFY(model->setData(model->index(row), Qt::Unchecked, Qt::CheckStateRole));
        QVERIFY(Settings::videoRecordedAudioSources().isEmpty());
        QVERIFY(Settings::videoRecordSystemAudio());
    }
};

QTEST_MAIN(AudioDeviceModelTest)

#include "AudioDeviceModelTest.moc"
