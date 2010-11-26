/* This file is (c) 2008-2011 Konstantin Isakov <ikm@users.berlios.de>
 * Part of GoldenDict. Licensed under GPLv3 or later, see the LICENSE file */

#include "article_maker.hh"
#include "config.hh"
#include "htmlescape.hh"
#include "utf8.hh"
#include "wstring_qt.hh"
#include <limits.h>
#include <QFile>
#include <QUrl>
#include "folding.hh"
#include "langcoder.hh"

using std::vector;
using std::string;
using gd::wstring;
using std::set;
using std::list;

ArticleMaker::ArticleMaker( vector< sptr< Dictionary::Class > > const & dictionaries_,
                            vector< Instances::Group > const & groups_,
                            QString const & displayStyle_ ):
  dictionaries( dictionaries_ ),
  groups( groups_ ),
  displayStyle( displayStyle_ )
{
}

void ArticleMaker::setDisplayStyle( QString const & st )
{
  displayStyle = st;
}

std::string ArticleMaker::makeHtmlHeader( QString const & word,
                                          QString const & icon ) const
{
  string result =
    "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"
    "<html><head>"
    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">";

  // Add a css stylesheet

  {
    QFile builtInCssFile( ":/article-style.css" );
    builtInCssFile.open( QFile::ReadOnly );
    QByteArray css = builtInCssFile.readAll();
    
    if ( displayStyle.size() )
    {
      // Load an additional stylesheet
      QFile builtInCssFile( QString( ":/article-style-st-%1.css" ).arg( displayStyle ) );
      builtInCssFile.open( QFile::ReadOnly );
      css += builtInCssFile.readAll();
    }

    QFile cssFile( Config::getUserCssFileName() );
  
    if ( cssFile.open( QFile::ReadOnly ) )
      css += cssFile.readAll();
    
    result += "<style type=\"text/css\" media=\"all\">\n";
    result += css.data();
    result += "</style>\n";
  }

  // Add print-only css
  
  {
    QFile builtInCssFile( ":/article-style-print.css" );
    builtInCssFile.open( QFile::ReadOnly );
    QByteArray css = builtInCssFile.readAll();

    QFile cssFile( Config::getUserCssPrintFileName() );

    if ( cssFile.open( QFile::ReadOnly ) )
      css += cssFile.readAll();
    
    result += "<style type=\"text/css\" media=\"print\">\n";
    result += css.data();
    result += "</style>\n";
  }
  
  result += "<title>" + Html::escape( Utf8::encode( gd::toWString( word ) ) ) + "</title>";

  // This doesn't seem to be much of influence right now, but we'll keep
  // it anyway.
  if ( icon.size() )
    result += "<link rel=\"icon\" type=\"image/png\" href=\"qrcx://localhost/flags/" + Html::escape( icon.toUtf8().data() ) + "\" />\n";

  result += "<script language=\"JavaScript\">"
            "function gdMakeArticleActive( newId ) {"
            "if ( gdCurrentArticle != 'gdfrom-' + newId ) {"
            "document.getElementById( gdCurrentArticle ).className = 'gdarticle';"
            "document.getElementById( 'gdfrom-' + newId ).className = 'gdarticle gdactivearticle';"
            "gdCurrentArticle = 'gdfrom-' + newId; eval( 'gdActivateAudioLink_' + newId + '();' ); } }"
            "</script>";

  result += "</head><body>";

  return result;
}

std::string ArticleMaker::makeNotFoundBody( QString const & word,
                                            QString const & group )
{
  string result( "<div class=\"gdnotfound\"><p>" );

  if ( word.size() )
    result += tr( "No translation for <b>%1</b> was found in group <b>%2</b>." ).
              arg( QString::fromUtf8( Html::escape( word.toUtf8().data() ).c_str() ) ).
              arg( QString::fromUtf8( Html::escape( group.toUtf8().data() ).c_str() ) ).
                toUtf8().data();
  else
    result += tr( "No translation was found in group <b>%1</b>." ).
              arg( QString::fromUtf8( Html::escape( group.toUtf8().data() ).c_str() ) ).
                toUtf8().data();

  result += "</p></div>";

  return result;
}

sptr< Dictionary::DataRequest > ArticleMaker::makeDefinitionFor(
  QString const & inWord, unsigned groupId,
  QMap< QString, QString > const & contexts,
  QSet< QString > const & mutedDicts ) const
{
  if ( groupId == Instances::Group::HelpGroupId )
  {
    // This is a special group containing internal welcome/help pages
    string result = makeHtmlHeader( inWord, QString() );
    
    if ( inWord == tr( "Welcome!" ) )
    {
      result += tr(
"<h3 align=\"center\">Welcome to <b>GoldenDict</b>!</h3>"
"<p>To start working with the program, first visit <b>Edit|Dictionaries</b> to add some directory paths where to search "
"for the dictionary files, set up various Wikipedia sites or other sources, adjust dictionary order or create dictionary groups."
"<p>And then you're ready to look up your words! You can do that in this window "
"by using a pane to the left, or you can <a href=\"Working with popup\">look up words from other active applications</a>. "
"<p>To customize program, check out the available preferences at <b>Edit|Preferences</b>. "
"All settings there have tooltips, be sure to read them if you are in doubt about anything."
"<p>Should you need further help, have any questions, "
"suggestions or just wonder what the others think, you are welcome at the program's <a href=\"http://goldendict.org/forum/\">forum</a>."
"<p>Check program's <a href=\"http://goldendict.org/\">website</a> for the updates. "
"<p>(c) 2008-2011 Konstantin Isakov. Licensed under GPLv3 or later."
        
        ).toUtf8().data();
    }
    else
    if ( inWord == tr( "Working with popup" ) )
    {
      result += ( tr( "<h3 align=\"center\">Working with the popup</h3>"

"To look up words from other active applications, you would need to first activate the <i>\"Scan popup functionality\"</i> in <b>Preferences</b>, "
"and then enable it at any time either by triggering the 'Popup' icon above, or "
"by clicking the tray icon down below with your right mouse button and choosing so in the menu you've popped. " ) +

#ifdef Q_OS_WIN32
  tr( "Then just stop the cursor over the word you want to look up in another application, "
       "and a window would pop up which would describe it to you." )
#else
  tr( "Then just select any word you want to look up in another application by your mouse "
      "(double-click it or swipe it with mouse with the button pressed), "
      "and a window would pop up which would describe the word to you." )
#endif
  ).toUtf8().data();
    }
    else
    {
      // Not found
      return makeNotFoundTextFor( inWord, "help" );
    }
    
    result += "</body></html>";

    sptr< Dictionary::DataRequestInstant > r = new Dictionary::DataRequestInstant( true );

    r->getData().resize( result.size() );
    memcpy( &( r->getData().front() ), result.data(), result.size() );

    return r;
  }

  // Find the given group

  Instances::Group const * activeGroup = 0;

  for( unsigned x = 0; x < groups.size(); ++x )
    if ( groups[ x ].id == groupId )
    {
      activeGroup = &groups[ x ];
      break;
    }

  // If we've found a group, use its dictionaries; otherwise, use the global
  // heap.
  std::vector< sptr< Dictionary::Class > > const & activeDicts =
    activeGroup ? activeGroup->dictionaries : dictionaries;

  string header = makeHtmlHeader( inWord.trimmed(),
                                  activeGroup && activeGroup->icon.size() ?
                                    activeGroup->icon : QString() );

  if ( mutedDicts.size() )
  {
    std::vector< sptr< Dictionary::Class > > unmutedDicts;

    unmutedDicts.reserve( activeDicts.size() );

    for( unsigned x = 0; x < activeDicts.size(); ++x )
      if ( !mutedDicts.contains(
              QString::fromStdString( activeDicts[ x ]->getId() ) ) )
        unmutedDicts.push_back( activeDicts[ x ] );

    return new ArticleRequest( inWord.trimmed(), activeGroup ? activeGroup->name : "",
                               contexts, unmutedDicts, header );
  }
  else
    return new ArticleRequest( inWord.trimmed(), activeGroup ? activeGroup->name : "",
                               contexts, activeDicts, header );
}

sptr< Dictionary::DataRequest > ArticleMaker::makeNotFoundTextFor(
  QString const & word, QString const & group ) const
{
  string result = makeHtmlHeader( word, QString() ) + makeNotFoundBody( word, group ) +
    "</body></html>";

  sptr< Dictionary::DataRequestInstant > r = new Dictionary::DataRequestInstant( true );

  r->getData().resize( result.size() );
  memcpy( &( r->getData().front() ), result.data(), result.size() );

  return r;
}

sptr< Dictionary::DataRequest > ArticleMaker::makeEmptyPage() const
{
  string result = makeHtmlHeader( tr( "(untitled)" ), QString() ) +
    "</body></html>";

  sptr< Dictionary::DataRequestInstant > r =
      new Dictionary::DataRequestInstant( true );

  r->getData().resize( result.size() );
  memcpy( &( r->getData().front() ), result.data(), result.size() );

  return r;
}

//////// ArticleRequest

ArticleRequest::ArticleRequest(
  QString const & word_, QString const & group_,
  QMap< QString, QString > const & contexts_,
  vector< sptr< Dictionary::Class > > const & activeDicts_,
  string const & header ):
    word( word_ ), group( group_ ), contexts( contexts_ ),
    activeDicts( activeDicts_ ),
    altsDone( false ), bodyDone( false ), foundAnyDefinitions( false ),
    closePrevSpan( false )
{
  // No need to lock dataMutex on construction

  hasAnyData = true;

  data.resize( header.size() );
  memcpy( &data.front(), header.data(), header.size() );

  // Accumulate main forms

  for( unsigned x = 0; x < activeDicts.size(); ++x )
  {
    sptr< Dictionary::WordSearchRequest > s = activeDicts[ x ]->findHeadwordsForSynonym( gd::toWString( word ) );

    connect( s.get(), SIGNAL( finished() ),
             this, SLOT( altSearchFinished() ) );

    altSearches.push_back( s );
  }

  altSearchFinished(); // Handle any ones which have already finished
}

void ArticleRequest::altSearchFinished()
{
  if ( altsDone )
    return;
  
  // Check every request for finishing
  for( list< sptr< Dictionary::WordSearchRequest > >::iterator i =
         altSearches.begin(); i != altSearches.end(); )
  {
    if ( (*i)->isFinished() )
    {
      // This one's finished
      for( size_t count = (*i)->matchesCount(), x = 0; x < count; ++x )
        alts.insert( (**i)[ x ].word );

      altSearches.erase( i++ );
    }
    else
      ++i;
  }

  if ( altSearches.empty() )
  {
    printf( "alts finished\n" );
    
    // They all've finished! Now we can look up bodies

    altsDone = true; // So any pending signals in queued mode won't mess us up

    vector< wstring > altsVector( alts.begin(), alts.end() );
    
    for( unsigned x = 0; x < altsVector.size(); ++x )
    {
      printf( "Alt: %ls\n", altsVector[ x ].c_str() );
    }

    wstring wordStd = gd::toWString( word );

    for( unsigned x = 0; x < activeDicts.size(); ++x )
    {
      sptr< Dictionary::DataRequest > r =
        activeDicts[ x ]->getArticle( wordStd, altsVector,
                                      gd::toWString( contexts.value( QString::fromStdString( activeDicts[ x ]->getId() ) ) ) );

      connect( r.get(), SIGNAL( finished() ),
               this, SLOT( bodyFinished() ) );

      bodyRequests.push_back( r );
    }

    bodyFinished(); // Handle any ones which have already finished
  }
}

void ArticleRequest::bodyFinished()
{
  if ( bodyDone )
    return;

  printf( "some body finished\n" );
  
  bool wasUpdated = false;
  
  while ( bodyRequests.size() )
  {
    // Since requests should go in order, check the first one first
    if ( bodyRequests.front()->isFinished() )
    {
      // Good

      printf( "one finished.\n" );

      Dictionary::DataRequest & req = *bodyRequests.front();

      QString errorString = req.getErrorString();

      if ( req.dataSize() >= 0 || errorString.size() )
      {
        sptr< Dictionary::Class > const & activeDict =
            activeDicts[ activeDicts.size() - bodyRequests.size() ];

        string dictId = activeDict->getId();
        
        string head;

        string gdFrom = "gdfrom-" + Html::escape( dictId );

        if ( closePrevSpan )
        {
          head += "</span></span><span class=\"gdarticleseparator\"></span>";
        }
        else
        {
          // This is the first article
          head += "<script language=\"JavaScript\">"
                  "var gdCurrentArticle=\"" + gdFrom  + "\";</script>";
        }

        string jsVal = Html::escapeForJavaScript( dictId );
        head += "<script language=\"JavaScript\">var gdArticleContents; "
          "if ( !gdArticleContents ) gdArticleContents = \"" + jsVal +" \"; "
          "else gdArticleContents += \"" + jsVal + " \";</script>";

        head += string( "<span class=\"gdarticle" ) +
                ( closePrevSpan ? "" : " gdactivearticle" ) +
                "\" id=\"" + gdFrom +
                "\" onClick=\"gdMakeArticleActive( '" + jsVal + "' );\" " +
                " onContextMenu=\"gdMakeArticleActive( '" + jsVal + "' );\""
                + ">";

        closePrevSpan = true;

        head += string( "<div class=\"gddictname\"><span class=\"gdfromprefix\">" ) +
          Html::escape( tr( "From " ).toUtf8().data() ) + "</span>" +
          Html::escape( activeDict->getName().c_str() )
           + "</div>";

        head += "<span class=\"gdarticlebody gdlangfrom-";
        head += LangCoder::intToCode2( activeDict->getLangFrom() ).toAscii().data();
        head += "\" lang=\"";
        head += LangCoder::intToCode2( activeDict->getLangTo() ).toAscii().data();
        head += "\">";

        if ( errorString.size() )
        {
          head += "<div class=\"gderrordesc\">" +
            Html::escape( tr( "Query error: %1" ).arg( errorString ).toUtf8().data() )
          + "</div>";
        }

        Mutex::Lock _( dataMutex );
  
        size_t offset = data.size();
        
        data.resize( data.size() + head.size() + ( req.dataSize() > 0 ? req.dataSize() : 0 ) );
  
        memcpy( &data.front() + offset, head.data(), head.size() );

        if ( req.dataSize() > 0 )
          bodyRequests.front()->getDataSlice( 0, req.dataSize(),
                                              &data.front() + offset + head.size() );

        wasUpdated = true;

        foundAnyDefinitions = true;
      }
      printf( "erasing..\n" );
      bodyRequests.pop_front();
      printf( "erase done..\n" );
    }
    else
    {
        printf( "one not finished.\n" );
        break;
    }
  }

  if ( bodyRequests.empty() )
  {
    // No requests left, end the article

    bodyDone = true;
    
    {
      string footer;

      if ( closePrevSpan )
      {
        footer += "</span></span>";
        closePrevSpan = false;
      }
      
      if ( !foundAnyDefinitions )
      {
        // No definitions were ever found, say so to the user.

        // Larger words are usually whole sentences - don't clutter the ouput
        // with their full bodies.
        footer += ArticleMaker::makeNotFoundBody( word.size() < 40 ? word : "", group );

        // When there were no definitions, we run stemmed search.
        stemmedWordFinder = new WordFinder( this );

        connect( stemmedWordFinder.get(), SIGNAL( finished() ),
                 this, SLOT( stemmedSearchFinished() ), Qt::QueuedConnection );

        stemmedWordFinder->stemmedMatch( word, activeDicts );
      }
      else
      {
        footer += "</body></html>";
      }

      Mutex::Lock _( dataMutex );
  
      size_t offset = data.size();
      
      data.resize( data.size() + footer.size() );
  
      memcpy( &data.front() + offset, footer.data(), footer.size() );
    }

    if ( stemmedWordFinder.get() )
      update();
    else
      finish();
  }
  else
  if ( wasUpdated )
    update();
}

void ArticleRequest::stemmedSearchFinished()
{
  // Got stemmed matching results

  WordFinder::SearchResults sr = stemmedWordFinder->getResults();

  string footer;

  bool continueMatching = false;

  if ( sr.size() )
  {
    footer += "<div class=\"gdstemmedsuggestion\"><span class=\"gdstemmedsuggestion_head\">" +
      Html::escape( tr( "Close words: " ).toUtf8().data() ) +
      "</span><span class=\"gdstemmedsuggestion_body\">";

    for( unsigned x = 0; x < sr.size(); ++x )
    {
      footer += linkWord( sr[ x ].first );

      if ( x != sr.size() - 1 )
      {
        footer += ", ";
      }
    }

    footer += "</span></div>";
  }

  splittedWords = splitIntoWords( word );

  if ( splittedWords.first.size() > 1 ) // Contains more than one word
  {
    disconnect( stemmedWordFinder.get(), SIGNAL( finished() ),
                this, SLOT( stemmedSearchFinished() ) );

    connect( stemmedWordFinder.get(), SIGNAL( finished() ),
             this, SLOT( individualWordFinished() ), Qt::QueuedConnection );

    currentSplittedWordStart = -1;
    currentSplittedWordEnd = currentSplittedWordStart;

    firstCompoundWasFound = false;

    compoundSearchNextStep( false );

    continueMatching = true;
  }

  if ( !continueMatching )
    footer += "</body></html>";

  {
    Mutex::Lock _( dataMutex );
  
    size_t offset = data.size();
  
    data.resize( data.size() + footer.size() );
  
    memcpy( &data.front() + offset, footer.data(), footer.size() );
  }

  if ( continueMatching )
    update();
  else
    finish();
}

void ArticleRequest::compoundSearchNextStep( bool lastSearchSucceeded )
{
  if ( !lastSearchSucceeded )
  {
    // Last search was unsuccessful. First, emit what we had.

    string footer;

    if ( lastGoodCompoundResult.size() ) // We have something to append
    {
//      printf( "Appending\n" );

      if ( !firstCompoundWasFound )
      {
        // Append the beginning
        footer += "<div class=\"gdstemmedsuggestion\"><span class=\"gdstemmedsuggestion_head\">" +
          Html::escape( tr( "Compound expressions: " ).toUtf8().data() ) +
          "</span><span class=\"gdstemmedsuggestion_body\">";

        firstCompoundWasFound = true;
      }
      else
      {
        // Append the separator
        footer += " / ";
      }

      footer += linkWord( lastGoodCompoundResult );

      lastGoodCompoundResult.clear();
    }

    // Then, start a new search for the next word, if possible

    if ( currentSplittedWordStart >= splittedWords.first.size() - 2 )
    {
      // The last word was the last possible to start from

      if ( firstCompoundWasFound )
        footer += "</span>";

      // Now add links to all the individual words. They conclude the result.

      footer += "<div class=\"gdstemmedsuggestion\"><span class=\"gdstemmedsuggestion_head\">" +
        Html::escape( tr( "Individual words: " ).toUtf8().data() ) +
        "</span><span class=\"gdstemmedsuggestion_body\">";

      footer += escapeSpacing( splittedWords.second[ 0 ] );

      for( int x = 0; x < splittedWords.first.size(); ++x )
      {
        footer += linkWord( splittedWords.first[ x ] );
        footer += escapeSpacing( splittedWords.second[ x + 1 ] );
      }

      footer += "</span>";

      footer += "</body></html>";

      appendToData( footer );

      finish();

      return;
    }

    if ( footer.size() )
    {
      appendToData( footer );
      update();
    }

    // Advance to the next word and start from looking up two words
    ++currentSplittedWordStart;
    currentSplittedWordEnd = currentSplittedWordStart + 1;
  }
  else
  {
    // Last lookup succeeded -- see if we can try the larger sequence

    if ( currentSplittedWordEnd < splittedWords.first.size() - 1 )
    {
      // We can, indeed.
      ++currentSplittedWordEnd;
    }
    else
    {
      // We can't. Emit what we have and start over.

      ++currentSplittedWordEnd; // So we could use the same code for result
                                // emitting

      // Initiate new lookup
      compoundSearchNextStep( false );

      return;
    }
  }

  // Build the compound sequence

  currentSplittedWordCompound = makeSplittedWordCompound();

  // Look it up

//  printf( "Looking up %s\n", qPrintable( currentSplittedWordCompound ) );

  stemmedWordFinder->prefixMatch( currentSplittedWordCompound, activeDicts, 40, // Would one be enough? Leave 40 to be safe.
                                  Dictionary::SuitableForCompoundSearching );
}

QString ArticleRequest::makeSplittedWordCompound()
{
  QString result;

  result.clear();

  for( int x = currentSplittedWordStart; x <= currentSplittedWordEnd; ++x )
  {
    result.append( splittedWords.first[ x ] );

    if ( x < currentSplittedWordEnd )
    {
      wstring ws( gd::toWString( splittedWords.second[ x + 1 ] ) );

      Folding::normalizeWhitespace( ws );

      result.append( gd::toQString( ws ) );
    }
  }

  return result;
}

void ArticleRequest::individualWordFinished()
{
  WordFinder::SearchResults const & results = stemmedWordFinder->getResults();

  if ( results.size() )
  {
    // Check if the aliases are acceptable
    wstring source = Folding::applySimpleCaseOnly( gd::toWString( currentSplittedWordCompound ) );

    bool hadSomething = false;

    for( unsigned x = 0; x < results.size(); ++x )
    {
      if ( results[ x ].second )
        continue; // We're not interested in suggestions

      wstring result( Folding::applySimpleCaseOnly( gd::toWString( results[ x ].first ) ) );

      if ( source.size() <= result.size() && result.compare( 0, source.size(), source ) == 0 )
      {
        // The resulting string begins with the source one

        hadSomething = true;

        if ( source.size() == result.size() )
        {
          // Got the match. No need to continue.
          lastGoodCompoundResult = currentSplittedWordCompound;
          break;
        }
      }
    }

    if ( hadSomething )
    {
      compoundSearchNextStep( true );
      return;
    }
  }

  compoundSearchNextStep( false );
}

void ArticleRequest::appendToData( std::string const & str )
{
  Mutex::Lock _( dataMutex );

  size_t offset = data.size();

  data.resize( data.size() + str.size() );

  memcpy( &data.front() + offset, str.data(), str.size() );

}

QPair< ArticleRequest::Words, ArticleRequest::Spacings > ArticleRequest::splitIntoWords( QString const & input )
{
  QPair< Words, Spacings > result;

  QChar const * ptr = input.data();

  for( ; ; )
  {
    QString spacing;

    for( ; ptr->unicode() && ( Folding::isPunct( ptr->unicode() ) || Folding::isWhitespace( ptr->unicode() ) ); ++ptr )
      spacing.append( *ptr );

    result.second.append( spacing );

    QString word;

    for( ; ptr->unicode() && !( Folding::isPunct( ptr->unicode() ) || Folding::isWhitespace( ptr->unicode() ) ); ++ptr )
      word.append( *ptr );

    if ( word.isEmpty() )
      break;

    result.first.append( word );
  }

  return result;
}

string ArticleRequest::linkWord( QString const & str )
{
  QUrl url;

  url.setScheme( "gdlookup" );
  url.setHost( "localhost" );
  url.setPath( str );

  string escapedResult = Html::escape( str.toUtf8().data() );
  return string( "<a href=\"" ) + url.toEncoded().data() + "\">" + escapedResult +"</a>";
}

std::string ArticleRequest::escapeSpacing( QString const & str )
{
  QByteArray spacing = Html::escape( str.toUtf8().data() ).c_str();

  spacing.replace( "\n", "<br>" );

  return spacing.data();
}