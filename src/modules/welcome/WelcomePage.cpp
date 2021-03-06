/* === This file is part of Calamares - <https://github.com/calamares> ===
 *
 *   Copyright 2014-2015, Teo Mrnjavac <teo@kde.org>
 *   Copyright 2015,      Anke Boersma <demm@kaosx.us>
 *   Copyright 2017-2018, Adriaan de Groot <groot@kde.org>
 *
 *   Calamares is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Calamares is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Calamares. If not, see <http://www.gnu.org/licenses/>.
 */

#include "WelcomePage.h"

#include "ui_WelcomePage.h"
#include "CalamaresVersion.h"
#include "checker/RequirementsChecker.h"
#include "utils/Logger.h"
#include "utils/CalamaresUtilsGui.h"
#include "utils/Retranslator.h"
#include "ViewManager.h"

#include <QApplication>
#include <QBoxLayout>
#include <QDesktopServices>
#include <QFocusEvent>
#include <QLabel>
#include <QComboBox>
#include <QMessageBox>

#include "Branding.h"


WelcomePage::WelcomePage( RequirementsChecker* requirementsChecker, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::WelcomePage )
    , m_requirementsChecker( requirementsChecker )
{
    ui->setupUi( this );

    ui->verticalLayout->insertSpacing( 1, CalamaresUtils::defaultFontHeight() * 2 );
    initLanguages();

    ui->mainText->setAlignment( Qt::AlignCenter );
    ui->mainText->setWordWrap( true );
    ui->mainText->setOpenExternalLinks( true );

    cDebug() << "Welcome string" << Calamares::Branding::instance()->welcomeStyleCalamares()
        << *Calamares::Branding::VersionedName;

    CALAMARES_RETRANSLATE(
        ui->mainText->setText( (Calamares::Branding::instance()->welcomeStyleCalamares() ? tr( "<h1>Welcome to the Calamares installer for %1.</h1>" ) : tr( "<h1>Welcome to the %1 installer.</h1>" ))
                                .arg( *Calamares::Branding::VersionedName ) );
        ui->retranslateUi( this );
    )

    ui->aboutButton->setIcon( CalamaresUtils::defaultPixmap( CalamaresUtils::Information,
                                                             CalamaresUtils::Original,
                                                             2*QSize( CalamaresUtils::defaultFontHeight(),
                                                                    CalamaresUtils::defaultFontHeight() ) ) );
    connect( ui->aboutButton, &QPushButton::clicked,
             this, [ this ]
    {
        QMessageBox mb( QMessageBox::Information,
                        tr( "About %1 installer" )
                            .arg( CALAMARES_APPLICATION_NAME ),
                        tr(
                            "<h1>%1</h1><br/>"
                            "<strong>%2<br/>"
                            "for %3</strong><br/><br/>"
                            "Copyright 2014-2017 Teo Mrnjavac &lt;teo@kde.org&gt;<br/>"
                            "Copyright 2017 Adriaan de Groot &lt;groot@kde.org&gt;<br/>"
                            "Thanks to: Anke Boersma, Aurélien Gâteau, Kevin Kofler, Lisa Vitolo,"
                            " Philip Müller, Pier Luigi Fiorini, Rohan Garg and the <a "
                            "href=\"https://www.transifex.com/calamares/calamares/\">Calamares "
                            "translators team</a>.<br/><br/>"
                            "<a href=\"https://calamares.io/\">Calamares</a> "
                            "development is sponsored by <br/>"
                            "<a href=\"http://www.blue-systems.com/\">Blue Systems</a> - "
                            "Liberating Software."
                        )
                        .arg( CALAMARES_APPLICATION_NAME )
                        .arg( CALAMARES_VERSION )
                        .arg( *Calamares::Branding::VersionedName ),
                        QMessageBox::Ok,
                        this );
        mb.setIconPixmap( CalamaresUtils::defaultPixmap( CalamaresUtils::Squid,
                                                         CalamaresUtils::Original,
                                                         QSize( CalamaresUtils::defaultFontHeight() * 6,
                                                                CalamaresUtils::defaultFontHeight() * 6 ) ) );
        QGridLayout* layout = reinterpret_cast<QGridLayout *>( mb.layout() );
        if ( layout )
            layout->setColumnMinimumWidth( 2, CalamaresUtils::defaultFontHeight() * 24 );
        mb.exec();
    } );

    ui->verticalLayout->insertWidget( 3, m_requirementsChecker->widget() );
}


/** @brief Match the combobox of languages with a predicate
 *
 * Scans the entries in the @p list (actually a ComboBox) and if one
 * matches the given @p predicate, returns true and sets @p matchFound
 * to the locale that matched.
 *
 * If none match, returns false and leaves @p matchFound unchanged.
 */
static
bool matchLocale( QComboBox& list, QLocale& matchFound, std::function<bool(const QLocale&)> predicate)
{
    for (int i = 0; i < list.count(); i++)
    {
        QLocale thisLocale = list.itemData( i, Qt::UserRole ).toLocale();
        if ( predicate(thisLocale) )
        {
            list.setCurrentIndex( i );
            cDebug() << " .. Matched locale " << thisLocale.name();
            matchFound = thisLocale;
            return true;
        }
    }

    return false;
}

struct LocaleLabel
{
    LocaleLabel( const QString& locale )
        : m_locale( LocaleLabel::getLocale( locale ) )
        , m_localeId( locale )
    {
        QString sortKey = QLocale::languageToString( m_locale.language() );
        QString label = m_locale.nativeLanguageName();

        if ( locale.contains( '_' ) && QLocale::countriesForLanguage( m_locale.language() ).count() > 2 )
        {
            QLatin1Literal countrySuffix( " (%1)" );

            sortKey.append( QString( countrySuffix ).arg( QLocale::countryToString( m_locale.country() ) ) );

            // If the language name is RTL, make this parenthetical addition RTL as well.
            QString countryFormat = label.isRightToLeft() ? QString( QChar( 0x202B ) ) : QString();
            countryFormat.append( countrySuffix );
            label.append( countryFormat.arg( m_locale.nativeCountryName() ) );
        }

        m_sortKey = sortKey;
        m_label = label;
    }

    QLocale m_locale;
    QString m_localeId;  // the locale identifier, e.g. "en_GB"
    QString m_sortKey;  // the English name of the locale
    QString m_label;  // the native name of the locale

    /** @brief Define a sorting order.
     *
     * English (@see isEnglish() -- it means en_US) is sorted at the top.
     */
    bool operator <(const LocaleLabel& other) const
    {
        if ( isEnglish() )
            return !other.isEnglish();
        if ( other.isEnglish() )
            return false;
        return m_sortKey < other.m_sortKey;
    }

    /** @brief Is this locale English?
     *
     * en_US and en (American English) is defined as English. The Queen's
     * English -- proper English -- is relegated to non-English status.
     */
    bool isEnglish() const
    {
       return m_localeId == QLatin1Literal( "en_US" ) || m_localeId == QLatin1Literal( "en" );
    }

    static QLocale getLocale( const QString& localeName )
    {
        if ( localeName.contains( "@latin" ) )
        {
            QLocale loc( localeName );
            return QLocale( loc.language(), QLocale::Script::LatinScript, loc.country() );
        }
        else
            return QLocale( localeName );
    }
} ;

void
WelcomePage::initLanguages()
{
    // Fill the list of translations
    ui->languageWidget->clear();
    ui->languageWidget->setInsertPolicy( QComboBox::InsertAtBottom );

    {
        std::list< LocaleLabel > localeList;
        const auto locales = QString( CALAMARES_TRANSLATION_LANGUAGES ).split( ';');
        for ( const QString& locale : locales )
        {
            localeList.emplace_back( locale );
        }

        localeList.sort(); // According to the sortkey, which is english

        for ( const auto& locale : localeList )
        {
            ui->languageWidget->addItem( locale.m_label, locale.m_locale );
        }
    }

    // Find the best initial translation
    QLocale defaultLocale = QLocale( QLocale::system().name() );
    QLocale matchedLocale;

    cDebug() << "Matching exact locale" << defaultLocale;
    bool isTranslationAvailable =
        matchLocale( *(ui->languageWidget), matchedLocale,
                      [&](const QLocale& x){ return x.language() == defaultLocale.language() && x.country() == defaultLocale.country(); } );

    if ( !isTranslationAvailable )
    {
        cDebug() << "Matching approximate locale" << defaultLocale.language();

        isTranslationAvailable =
            matchLocale( *(ui->languageWidget), matchedLocale,
                          [&](const QLocale& x){ return x.language() == defaultLocale.language(); } ) ;
    }

    if ( !isTranslationAvailable )
    {
        QLocale en_us( QLocale::English, QLocale::UnitedStates );

        cDebug() << "Matching English (US)";
        isTranslationAvailable =
            matchLocale( *(ui->languageWidget), matchedLocale,
                          [&](const QLocale& x){ return x == en_us; } );

        // Now, if it matched, because we didn't match the system locale, switch to the one found
        if ( isTranslationAvailable )
            QLocale::setDefault( matchedLocale );
    }

    if ( isTranslationAvailable )
        CalamaresUtils::installTranslator( matchedLocale.name(),
                                           Calamares::Branding::instance()->translationsPathPrefix(),
                                           qApp );
    else
        cWarning() << "No available translation matched" << defaultLocale;

    connect( ui->languageWidget,
             static_cast< void ( QComboBox::* )( int ) >( &QComboBox::currentIndexChanged ),
             this,
             [&]( int newIndex )
             {
                 QLocale selectedLocale = ui->languageWidget->itemData( newIndex, Qt::UserRole ).toLocale();
                 cDebug() << "Selected locale" << selectedLocale;

                 QLocale::setDefault( selectedLocale );
                 CalamaresUtils::installTranslator( selectedLocale,
                                                    Calamares::Branding::instance()->translationsPathPrefix(),
                                                    qApp );
             } );
}


void
WelcomePage::setUpLinks( bool showSupportUrl,
                          bool showKnownIssuesUrl,
                          bool showReleaseNotesUrl )
{
    using namespace Calamares;
    if ( showSupportUrl && !( *Branding::SupportUrl ).isEmpty() )
    {
        CALAMARES_RETRANSLATE(
            ui->supportButton->setText( tr( "%1 support" )
                                        .arg( *Branding::ShortProductName ) );
        )
        ui->supportButton->setIcon( CalamaresUtils::defaultPixmap( CalamaresUtils::Help,
                                                                   CalamaresUtils::Original,
                                                                   2*QSize( CalamaresUtils::defaultFontHeight(),
                                                                          CalamaresUtils::defaultFontHeight() ) ) );
        connect( ui->supportButton, &QPushButton::clicked, []
        {
            QDesktopServices::openUrl( *Branding::SupportUrl );
        } );
    }
    else
    {
        ui->supportButton->hide();
    }

    if ( showKnownIssuesUrl && !( *Branding::KnownIssuesUrl ).isEmpty() )
    {
        ui->knownIssuesButton->setIcon( CalamaresUtils::defaultPixmap( CalamaresUtils::Bugs,
                                                                       CalamaresUtils::Original,
                                                                       2*QSize( CalamaresUtils::defaultFontHeight(),
                                                                              CalamaresUtils::defaultFontHeight() ) ) );
        connect( ui->knownIssuesButton, &QPushButton::clicked, []
        {
            QDesktopServices::openUrl( *Branding::KnownIssuesUrl );
        } );
    }
    else
    {
        ui->knownIssuesButton->hide();
    }

    if ( showReleaseNotesUrl && !( *Branding::ReleaseNotesUrl ).isEmpty() )
    {
        ui->releaseNotesButton->setIcon( CalamaresUtils::defaultPixmap( CalamaresUtils::Release,
                                                                        CalamaresUtils::Original,
                                                                        2*QSize( CalamaresUtils::defaultFontHeight(),
                                                                               CalamaresUtils::defaultFontHeight() ) ) );
        connect( ui->releaseNotesButton, &QPushButton::clicked, []
        {
            QDesktopServices::openUrl( *Branding::ReleaseNotesUrl );
        } );
    }
    else
    {
        ui->releaseNotesButton->hide();
    }
}


void
WelcomePage::focusInEvent( QFocusEvent* e )
{
    if ( ui->languageWidget )
        ui->languageWidget->setFocus();
    e->accept();
}

