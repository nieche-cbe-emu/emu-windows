
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QLineEdit>
#include <QListWidget>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QtTest>

#include "mainwindow.h"

namespace {
constexpr unsigned OK_MASK = (1u << 5) | (1u << 14);
constexpr unsigned UP_MASK = (1u << 2) | (1u << 17);
constexpr unsigned LSK_MASK = 1u << 12;
constexpr unsigned K1_MASK = 1u << 19;
}

class KeysTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void gameKeysReachEmulator_data();
    void gameKeysReachEmulator();
    void arrowsDoNotMoveLibrary();
    void enterDoesNotActivateLibraryItem();
    void modalDialogStillGetsTyping();
    void shortcutsWithModifiersPassThrough();

private:

    QTemporaryDir home;
};

void KeysTest::initTestCase()
{
    QVERIFY(home.isValid());
    QDir d(home.path());
    QVERIFY(d.mkpath(QStringLiteral("games")));
    for (const char *n : {"aaa.cbe", "bbb.cbe"}) {
        QFile f(d.filePath(QStringLiteral("games/%1").arg(QLatin1String(n))));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("not a real module");
    }
    qputenv("NIECHE_HOME", QDir::toNativeSeparators(home.path()).toLocal8Bit());
}

void KeysTest::gameKeysReachEmulator_data()
{
    QTest::addColumn<int>("key");
    QTest::addColumn<unsigned>("mask");
    QTest::newRow("J") << int(Qt::Key_J) << OK_MASK;
    QTest::newRow("空格") << int(Qt::Key_Space) << OK_MASK;
    QTest::newRow("回车") << int(Qt::Key_Return) << OK_MASK;
    QTest::newRow("小键盘回车") << int(Qt::Key_Enter) << OK_MASK;
    QTest::newRow("W") << int(Qt::Key_W) << UP_MASK;
    QTest::newRow("↑") << int(Qt::Key_Up) << UP_MASK;
    QTest::newRow("K") << int(Qt::Key_K) << LSK_MASK;
    QTest::newRow("1") << int(Qt::Key_1) << K1_MASK;
}

void KeysTest::gameKeysReachEmulator()
{
    QFETCH(int, key);
    QFETCH(unsigned, mask);

    MainWindow w;
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    auto *lib = w.findChild<QListWidget *>();
    QVERIFY(lib);
    QCOMPARE(lib->count(), 2);

    lib->setFocus();
    QCOMPARE(w.keyboardMask(), 0u);

    QTest::keyPress(lib, Qt::Key(key));
    QCOMPARE(w.keyboardMask(), mask);
    QTest::keyRelease(lib, Qt::Key(key));
    QCOMPARE(w.keyboardMask(), 0u);
}

void KeysTest::arrowsDoNotMoveLibrary()
{
    MainWindow w;
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    auto *lib = w.findChild<QListWidget *>();
    QVERIFY(lib);
    lib->setCurrentRow(0);
    lib->setFocus();

    QTest::keyClick(lib, Qt::Key_Down);
    QCOMPARE(lib->currentRow(), 0);
}

void KeysTest::enterDoesNotActivateLibraryItem()
{
    MainWindow w;
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    auto *lib = w.findChild<QListWidget *>();
    QVERIFY(lib);
    lib->setCurrentRow(0);
    lib->setFocus();
    QSignalSpy activated(lib, &QListWidget::itemActivated);

    QTest::keyClick(lib, Qt::Key_Return);
    QCOMPARE(activated.count(), 0);
}

void KeysTest::modalDialogStillGetsTyping()
{
    MainWindow w;
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QDialog dlg(&w);
    auto *edit = new QLineEdit(&dlg);
    auto *box = new QVBoxLayout(&dlg);
    box->addWidget(edit);
    dlg.setModal(true);
    dlg.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dlg));
    QCOMPARE(QApplication::activeModalWidget(), static_cast<QWidget *>(&dlg));

    edit->setFocus();
    QTest::keyClicks(edit, QStringLiteral("25w"));
    QCOMPARE(edit->text(), QStringLiteral("25w"));
    QCOMPARE(w.keyboardMask(), 0u);
    dlg.close();
}

void KeysTest::shortcutsWithModifiersPassThrough()
{
    MainWindow w;
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    auto *lib = w.findChild<QListWidget *>();
    QVERIFY(lib);
    lib->setFocus();

    QTest::keyPress(lib, Qt::Key_Space, Qt::AltModifier);
    QCOMPARE(w.keyboardMask(), 0u);
    QTest::keyRelease(lib, Qt::Key_Space, Qt::AltModifier);

    QTest::keyPress(lib, Qt::Key_W, Qt::ControlModifier);
    QCOMPARE(w.keyboardMask(), 0u);
    QTest::keyRelease(lib, Qt::Key_W, Qt::ControlModifier);

    QTest::keyPress(lib, Qt::Key_W, Qt::ShiftModifier);
    QCOMPARE(w.keyboardMask(), UP_MASK);
    QTest::keyRelease(lib, Qt::Key_W, Qt::ShiftModifier);
    QCOMPARE(w.keyboardMask(), 0u);
}

QTEST_MAIN(KeysTest)
#include "keys_test.moc"
