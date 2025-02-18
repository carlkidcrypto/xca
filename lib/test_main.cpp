/* vi: set sw=4 ts=4:
 *
 * Copyright (C) 2023 Christian Hohnstaedt.
 *
 * All rights reserved.
 */

#include <QDir>
#include <QDebug>
#include <QTest>
#include <QString>
#include <QThread>
#include <QApplication>

#include "widgets/MainWindow.h"
#include "ui_MainWindow.h"
#include "widgets/NewKey.h"
#include "ui_NewKey.h"
#include "entropy.h"
#include "pki_evp.h"
#include "debug_info.h"

char segv_data[1024];

class test_main: public QObject
{
    Q_OBJECT
	Entropy *entropy {};

	void openDB();

  private slots:
	void initTestCase();
	void cleanupTestCase();
	void newKey();
};

void test_main::initTestCase()
{
	debug_info::init();

	entropy = new Entropy;

	Settings.clear();
	initOIDs();

	mainwin = new MainWindow();
	mainwin->show();
}

void test_main::cleanupTestCase()
{
	Database.close();
	delete entropy;
	delete mainwin;
	pki_export::free_elements();
}

void test_main::openDB()
{
	pki_evp::passwd = "pass";
	QString salt = Entropy::makeSalt();
    pki_evp::passHash = pki_evp::sha512passwT(pki_evp::passwd, salt);
    Settings["pwhash"] = pki_evp::passHash;
	Database.open("testdb.xdb");
}

void test_main::newKey()
{
	/* RSA 3012 bit key - Remember as default */
	NewKey *dlg = new NewKey(mainwin, "Alfons");
	dlg->show();
	Q_ASSERT(QTest::qWaitForWindowActive(dlg));
	dlg->keyLength->setEditText("3012 bit");
	QCOMPARE(dlg->rememberDefault->isChecked(), false);
	dlg->rememberDefault->setChecked(true);
	dlg->accept();
	keyjob job = dlg->getKeyJob();
	QCOMPARE(job.ktype.name, "RSA");
	QCOMPARE(job.size, 3012);
	delete dlg;

	/* Remembered RSA:3012 key. Change to EC:secp521r1 */
	dlg = new NewKey(mainwin, "Erwin");
	dlg->show();
	Q_ASSERT(QTest::qWaitForWindowActive(dlg));
	QCOMPARE(dlg->rememberDefault->isChecked(), false);
	QCOMPARE(job.toString(), dlg->getKeyJob().toString());
#ifndef OPENSSL_NO_EC
	/* Curve box visible after selecting EC Key */
	QCOMPARE(dlg->curveBox->isVisible(),false);
	QCOMPARE(dlg->curveLabel->isVisible(),false);
	dlg->keyType->setCurrentIndex(2);
	QCOMPARE(dlg->curveBox->isVisible(),true);
	QCOMPARE(dlg->curveLabel->isVisible(),true);
	dlg->curveBox->setCurrentIndex(2);
	QCOMPARE(dlg->getKeyJob().toString(), "EC:secp521r1");
#ifdef EVP_PKEY_ED25519
	/* Select Edwards Curve */
	dlg->keyType->setCurrentIndex(3);
	QCOMPARE(dlg->getKeyJob().toString(), "ED25519");
	/* Neither key size nor curve is visible */
	QCOMPARE(dlg->curveBox->isVisible(),false);
	QCOMPARE(dlg->curveLabel->isVisible(),false);
	QCOMPARE(dlg->keyLength->isVisible(),false);
	QCOMPARE(dlg->keySizeLabel->isVisible(),false);
#endif
	/* Back to EC and previously set curve is set */
	dlg->keyType->setCurrentIndex(2);
	QCOMPARE(dlg->getKeyJob().toString(), "EC:secp521r1");
#endif
	dlg->accept();
	delete dlg;

	/* Open dialog again and RSA:3012 is remembered */
	dlg = new NewKey(mainwin, "Otto");
	dlg->show();
	Q_ASSERT(QTest::qWaitForWindowActive(dlg));
	QCOMPARE(dlg->rememberDefault->isChecked(), false);
	QCOMPARE(job.toString(), dlg->getKeyJob().toString());
	QCOMPARE(dlg->curveBox->isVisible(),false);
	QCOMPARE(dlg->curveLabel->isVisible(),false);
#ifndef OPENSSL_NO_EC
	/* Select EC and remember as default */
	dlg->keyType->setCurrentIndex(2);
	dlg->curveBox->setCurrentIndex(2);

	QCOMPARE(dlg->curveBox->isVisible(),true);
	QCOMPARE(dlg->curveLabel->isVisible(),true);
	QCOMPARE(dlg->getKeyJob().toString(), "EC:secp521r1");
	dlg->rememberDefault->setChecked(true);
	dlg->accept();
	delete dlg;

	/* Now "EC:secp521r1" is remembered as default */
	dlg = new NewKey(mainwin, "Heini");
	dlg->show();
	Q_ASSERT(QTest::qWaitForWindowActive(dlg));
	QCOMPARE(dlg->getKeyJob().toString(), "EC:secp521r1");
	QCOMPARE(dlg->rememberDefault->isChecked(), false);
#endif
	dlg->accept();
	delete dlg;
}

QTEST_MAIN(test_main)
#include "test_main.moc"
