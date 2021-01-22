// copyright (c) 2020-2021 hors<horsicq@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#include "xabstracttableview.h"

XAbstractTableView::XAbstractTableView(QWidget *pParent) : QAbstractScrollArea(pParent)
{
    g_bMouseResizeColumn=false;
    g_bMouseSelection=false;
    g_nViewStart=0;
    g_nCharWidth=0;
    g_nCharHeight=0;
    g_nLinesProPage=0;
    g_nLineHeight=0;
    g_nTotalLineCount=0;
    g_nViewWidth=0;
    g_nViewHeight=0;
    g_nTableWidth=0;
    g_nSelectionInitOffset=-1;
    g_nNumberOfRows=0;
    g_nCursorDelta=0;
    g_nXOffset=0;
    g_nHeaderHeight=20;
    g_nLineDelta=4; // TODO Check
    g_state={};
    g_bBlink=false;
    g_bLastColumnScretch=false;
    g_bHeaderVisible=false;
    g_bColumnFixed=false;
    g_bLinesVisible=false;

    g_nInitColumnNumber=0;

    g_pShortcuts=&scEmpty;

    installEventFilter(this);

    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(_customContextMenu(QPoint)));

    connect(verticalScrollBar(),SIGNAL(valueChanged(int)),this,SLOT(verticalScroll()));
    connect(horizontalScrollBar(),SIGNAL(valueChanged(int)),this,SLOT(horisontalScroll()));

    connect(&g_timerCursor,SIGNAL(timeout()),this,SLOT(updateBlink()));

    g_timerCursor.setInterval(500);
    g_timerCursor.start();

    setMouseTracking(true); // Important

    setHeaderVisible(true);
    setColumnFixed(false);
    setLinesVisible(true);
    // TODO Cursor off default
}

XAbstractTableView::~XAbstractTableView()
{

}

void XAbstractTableView::addColumn(QString sTitle, qint32 nWidth)
{
    COLUMN column={};

    column.bEnable=true;
    column.nWidth=nWidth;
    column.sTitle=sTitle;

    g_listColumns.append(column);
}

void XAbstractTableView::setColumnEnabled(qint32 nNumber, bool bState)
{
    if(nNumber<g_listColumns.count())
    {
        g_listColumns[nNumber].bEnable=bState;
    }
}

void XAbstractTableView::setColumnWidth(qint32 nNumber, qint32 nWidth)
{
    if(nNumber<g_listColumns.count())
    {
        g_listColumns[nNumber].nWidth=nWidth;
    }
}

void XAbstractTableView::paintEvent(QPaintEvent *pEvent)
{
    QPainter *pPainter=new QPainter(this->viewport());
    pPainter->setFont(g_fontText);
    pPainter->setPen(viewport()->palette().color(QPalette::WindowText));
    pPainter->setBackgroundMode(Qt::TransparentMode);

    if(g_rectCursor!=pEvent->rect())
    {
        startPainting(pPainter);

        int nNumberOfColumns=g_listColumns.count();

        if(nNumberOfColumns)
        {
            qint32 nTopLeftY=pEvent->rect().topLeft().y();
            qint32 nTopLeftX=pEvent->rect().topLeft().x()-g_nXOffset;
            qint32 nScreenWidth=pEvent->rect().width();

            qint32 nHeight=pEvent->rect().height();

            qint32 nX=nTopLeftX;

            qint32 nHeaderHeight=(g_bHeaderVisible)?(g_nHeaderHeight):(0);

            for(int i=0;i<nNumberOfColumns;i++)
            {
                if(g_listColumns.at(i).bEnable)
                {
                    qint32 nColumnWidth=g_listColumns.at(i).nWidth;

                    pPainter->fillRect(nX,nTopLeftY+nHeaderHeight,nColumnWidth,nHeight-nHeaderHeight,viewport()->palette().color(QPalette::Base));

                    paintColumn(pPainter,i,nX,nTopLeftY+nHeaderHeight,nColumnWidth,nHeight-nHeaderHeight);

                    for(int j=0;j<g_nLinesProPage;j++)
                    {
                        paintCell(pPainter,j,i,nX,nTopLeftY+nHeaderHeight+(j*g_nLineHeight),nColumnWidth,g_nLineHeight);
                    }

                    nX+=nColumnWidth;
                }
            }

            // Rest
            pPainter->fillRect(nX,nTopLeftY,nScreenWidth-nX,nHeight,viewport()->palette().color(QPalette::Base));

            nX=nTopLeftX;

            // Draw lines and headers
            for(int i=0;i<nNumberOfColumns;i++)
            {
                if(g_listColumns.at(i).bEnable)
                {
                    qint32 nColumnWidth=g_listColumns.at(i).nWidth;

                    if(nHeaderHeight>0)
                    {
                        QStyleOptionButton styleOptionButton;
                        styleOptionButton.state=QStyle::State_Enabled;

                        styleOptionButton.rect=QRect(nX,nTopLeftY,nColumnWidth,nHeaderHeight);

                        pushButtonHeader.style()->drawControl(QStyle::CE_PushButton,&styleOptionButton,pPainter,&pushButtonHeader);

                        QRect rect=QRect(nX+4,nTopLeftY,nColumnWidth-8,nHeaderHeight);
                        pPainter->drawText(rect,Qt::AlignVCenter|Qt::AlignLeft,g_listColumns.at(i).sTitle); // TODO alignment
                    }

                    if(g_bLinesVisible)
                    {
                        pPainter->drawLine(nX+nColumnWidth,nTopLeftY+nHeaderHeight,nX+nColumnWidth,nTopLeftY+nHeight);
                    }

                    nX+=nColumnWidth;
                }
            }
        }

        endPainting(pPainter);
    }

    // Draw cursor
    // TODO Cursor off
    if(g_rectCursor.width()&&g_rectCursor.height())
    {
        // TODO bold
        if(g_bBlink&&hasFocus())
        {
            pPainter->setPen(viewport()->palette().color(QPalette::Highlight));
            pPainter->fillRect(g_rectCursor,this->palette().color(QPalette::WindowText));
        }
        else
        {
            pPainter->setPen(viewport()->palette().color(QPalette::WindowText));
            pPainter->fillRect(g_rectCursor,this->palette().color(QPalette::Base));
        }

        pPainter->drawText(g_rectCursor.x(),g_rectCursor.y()+g_nLineHeight-g_nLineDelta,g_sCursorText);
    }

    delete pPainter;
}

bool XAbstractTableView::eventFilter(QObject *pObj, QEvent *pEvent)
{
    Q_UNUSED(pObj)

    if(pEvent->type()==QEvent::FocusIn)
    {
//        qDebug("QEvent::FocusIn");
        registerShortcuts(true);
    }
    else if(pEvent->type()==QEvent::FocusOut)
    {
//        qDebug("QEvent::FocusOut");
        registerShortcuts(false);
    }

    return false;
}

void XAbstractTableView::reload(bool bUpdateData)
{
    adjust(bUpdateData);
    viewport()->update();
}

void XAbstractTableView::setTextFont(const QFont &font)
{
    const QFontMetricsF fm(font);
    g_nCharWidth=fm.maxWidth();
    g_nCharHeight=fm.height();

    g_fontText=font;

    adjustColumns();
    adjust();
    viewport()->update();
}

QFont XAbstractTableView::getTextFont()
{
    return g_fontText;
}

void XAbstractTableView::setTotalLineCount(qint64 nValue)
{
    qint32 nScrollValue=0;

    // TODO fix scroll for the large files
    // mb flag for large files
    if(nValue>getMaxScrollValue())
    {
        nScrollValue=(qint32)getMaxScrollValue();
    }
    else
    {
        nScrollValue=(qint32)nValue;
    }

    verticalScrollBar()->setRange(0,nScrollValue);

    g_nTotalLineCount=nValue;
}

quint64 XAbstractTableView::getTotalLineCount()
{
    return g_nTotalLineCount;
}

qint32 XAbstractTableView::getLinesProPage()
{
    return g_nLinesProPage;
}

void XAbstractTableView::setViewStart(qint64 nValue)
{
    g_nViewStart=nValue;
}

qint64 XAbstractTableView::getViewStart()
{
    return g_nViewStart;
}

qint32 XAbstractTableView::getCharWidth()
{
    return g_nCharWidth;
}

XAbstractTableView::CURSOR_POSITION XAbstractTableView::getCursorPosition(QPoint pos)
{
    CURSOR_POSITION result={};

    result.nY=pos.y();
    result.nX=pos.x();

    qint32 nHeaderHeight=(g_bHeaderVisible)?(g_nHeaderHeight):(0);

    qint32 nCurrentOffset=0;
    int nNumberOfColumns=g_listColumns.count();

    for(int i=0;i<nNumberOfColumns;i++)
    {
        if(g_listColumns.at(i).bEnable)
        {
            if((result.nX>=nCurrentOffset)&&(result.nX<(nCurrentOffset+g_listColumns.at(i).nWidth)))
            {
                result.bIsValid=true;
                result.nColumn=i;

                if(result.nY<nHeaderHeight)
                {
                    result.ptype=PT_HEADER;
                }
                else
                {
                    result.ptype=PT_CELL;
                    result.nRow=(result.nY-nHeaderHeight)/g_nLineHeight;
                    result.nCellTop=(result.nY-nHeaderHeight)%g_nLineHeight;
                    result.nCellLeft=result.nX-nCurrentOffset;
                }

                if(result.nX>=(nCurrentOffset+g_listColumns.at(i).nWidth-g_nLineDelta))
                {
                    if(!g_bColumnFixed)
                    {
                        result.bResizeColumn=true;
                    }
                }

                break;
            }

            nCurrentOffset+=g_listColumns.at(i).nWidth;
        }
    }

    return result;
}

bool XAbstractTableView::isOffsetSelected(qint64 nOffset)
{
    bool bResult=false;

    if((nOffset>=g_state.nSelectionOffset)&&(nOffset<(g_state.nSelectionOffset+g_state.nSelectionSize)))
    {
        bResult=true;
    }

    return bResult;
}

qint32 XAbstractTableView::getLineDelta()
{
    return g_nLineDelta;
}

XAbstractTableView::STATE XAbstractTableView::getState()
{
    return g_state;
}

qint64 XAbstractTableView::getCursorOffset()
{
    return g_state.nCursorOffset;
}

void XAbstractTableView::setCursorOffset(qint64 nValue, qint32 nColumn)
{
    g_state.nCursorOffset=nValue;

    if(nColumn!=-1)
    {
        g_state.cursorPosition.nColumn=nColumn;
    }
}

void XAbstractTableView::_initSelection(qint64 nOffset)
{
    if(isOffsetValid(nOffset)||isEnd(nOffset))
    {
        g_nSelectionInitOffset=nOffset;
        g_state.nSelectionOffset=nOffset;
        g_state.nSelectionSize=0;
    }
}

void XAbstractTableView::_setSelection(qint64 nOffset)
{
    if(isOffsetValid(nOffset)||isEnd(nOffset))
    {
        if(nOffset>g_nSelectionInitOffset)
        {
            g_state.nSelectionOffset=g_nSelectionInitOffset;
            g_state.nSelectionSize=nOffset-g_nSelectionInitOffset;
        }
        else
        {
            g_state.nSelectionOffset=nOffset;
            g_state.nSelectionSize=g_nSelectionInitOffset-nOffset;
        }
    }
}

void XAbstractTableView::verticalScroll()
{
    g_nViewStart=getScrollValue();

    adjust(true);
}

void XAbstractTableView::horisontalScroll()
{
    adjust();
}

void XAbstractTableView::adjust(bool bUpdateData)
{
    g_nViewWidth=viewport()->width();
    g_nViewHeight=viewport()->height();

    g_nLineHeight=g_nCharHeight+5;

    qint32 nHeaderHeight=(g_bHeaderVisible)?(g_nHeaderHeight):(0);

    qint32 nLinesProPage=(g_nViewHeight-nHeaderHeight)/g_nLineHeight;

    if(nLinesProPage<0)
    {
        nLinesProPage=0;
    }

    if(g_nLinesProPage!=nLinesProPage)
    {
        bUpdateData=true;
        g_nLinesProPage=nLinesProPage;
    }

    g_nTableWidth=0;
    int nNumberOfColumns=g_listColumns.count();

    if(g_bLastColumnScretch)
    {
        if(nNumberOfColumns)
        {
            nNumberOfColumns--;
        }
    }

    for(int i=0;i<nNumberOfColumns;i++)
    {
        if(g_listColumns.at(i).bEnable)
        {
            g_listColumns[i].nLeft=g_nTableWidth;
            g_nTableWidth+=g_listColumns.at(i).nWidth;
        }
    }

    qint32 nDelta=g_nTableWidth-g_nViewWidth;

    if(g_bLastColumnScretch)
    {
        if(nDelta<0)
        {
            g_listColumns[nNumberOfColumns].nWidth=-(nDelta);
        }
        else
        {
            g_listColumns[nNumberOfColumns].nWidth=0;
        }
    }

    horizontalScrollBar()->setRange(0,nDelta);
    horizontalScrollBar()->setPageStep(g_nViewWidth);

    g_nXOffset=horizontalScrollBar()->value();

    if(bUpdateData)
    {
        updateData();
    }

//    resetCursor(); // TODO Check

    // TODO
}

void XAbstractTableView::adjustColumns()
{
    // TODO
}

void XAbstractTableView::registerShortcuts(bool bState)
{
    Q_UNUSED(bState)
}

void XAbstractTableView::setCursorData(QRect rect, QString sText, qint32 nDelta)
{
    g_rectCursor=rect;
    g_sCursorText=sText;
    g_nCursorDelta=nDelta;
}

void XAbstractTableView::resetCursorData()
{
    setCursorData(QRect(),"",0);
}

qint32 XAbstractTableView::getCursorDelta()
{
    return g_nCursorDelta;
}

void XAbstractTableView::setSelection(qint64 nOffset, qint64 nSize)
{
    _initSelection(nOffset);
    _setSelection(nOffset+nSize);

    adjust();
    viewport()->update();
}

qint64 XAbstractTableView::getMaxScrollValue()
{
    return 0x7FFFFFFF;
}

void XAbstractTableView::setLastColumnScretch(bool bState)
{
    g_bLastColumnScretch=bState;
}

void XAbstractTableView::setHeaderVisible(bool bState)
{
    g_bHeaderVisible=bState;
}

void XAbstractTableView::setColumnFixed(bool bState)
{
    g_bColumnFixed=bState;
}

void XAbstractTableView::setLinesVisible(bool bState)
{
    g_bLinesVisible=bState;
}

QFont XAbstractTableView::getMonoFont(qint32 nFontSize)
{
    QFont fontResult;
#ifdef Q_OS_WIN
    fontResult=QFont("Courier",nFontSize);
#endif
#ifdef Q_OS_LINUX
    fontResult=QFont("Monospace",nFontSize);
#endif
#ifdef Q_OS_OSX
    fontResult=QFont("Courier",nFontSize); // TODO Check "Menlo"
#endif

    return fontResult;
}

void XAbstractTableView::setShortcuts(XShortcuts *pShortcuts)
{
    g_pShortcuts=pShortcuts;
}

XShortcuts *XAbstractTableView::getShortcuts()
{
    return g_pShortcuts;
}

void XAbstractTableView::_customContextMenu(const QPoint &pos)
{
    contextMenu(mapToGlobal(pos));
}

void XAbstractTableView::updateBlink()
{
    g_bBlink=(bool)(!g_bBlink);
    viewport()->update(g_rectCursor);
}

void XAbstractTableView::resizeEvent(QResizeEvent *pEvent)
{
    adjust();
    QAbstractScrollArea::resizeEvent(pEvent);
}

void XAbstractTableView::mouseMoveEvent(QMouseEvent *pEvent)
{
    CURSOR_POSITION cursorPosition=getCursorPosition(pEvent->pos());

    if(g_bMouseSelection)
    {
        qint64 nOffset=cursorPositionToOffset(cursorPosition);

        if(nOffset!=-1)
        {
            g_state.nCursorOffset=nOffset;
            g_state.cursorPosition=cursorPosition;
            _setSelection(nOffset);

            adjust();
            viewport()->update();
        }
    }
    else if(g_bMouseResizeColumn)
    {
        qint32 nColumnWidth=qMax(g_nLineDelta,cursorPosition.nX-g_listColumns.at(g_nInitColumnNumber).nLeft);

        g_listColumns[g_nInitColumnNumber].nWidth=nColumnWidth;

        adjust();
        viewport()->update();
    }
    else if(pEvent->button()==Qt::NoButton)
    {
        if(cursorPosition.bResizeColumn)
        {
            setCursor(Qt::SplitHCursor);
        }
        else
        {
            unsetCursor();
        }
    }

    QAbstractScrollArea::mouseMoveEvent(pEvent);
}

void XAbstractTableView::mousePressEvent(QMouseEvent *pEvent)
{
    if(pEvent->button()==Qt::LeftButton)
    {
        CURSOR_POSITION cursorPosition=getCursorPosition(pEvent->pos());
        qint64 nOffset=cursorPositionToOffset(cursorPosition);

        if(cursorPosition.bResizeColumn)
        {
            g_bMouseResizeColumn=true;
            g_nInitColumnNumber=cursorPosition.nColumn;
            setCursor(Qt::SplitHCursor);
        }
        else if(nOffset!=-1)
        {
            g_state.nCursorOffset=nOffset;
            g_state.cursorPosition=cursorPosition;
            _initSelection(nOffset);
            g_bMouseSelection=true;
        }

        adjust();
        viewport()->update();
    }

    QAbstractScrollArea::mousePressEvent(pEvent);
}

void XAbstractTableView::mouseReleaseEvent(QMouseEvent *pEvent)
{
    Q_UNUSED(pEvent)

    g_bMouseResizeColumn=false;
    g_bMouseSelection=false;
}

void XAbstractTableView::keyPressEvent(QKeyEvent *pEvent)
{
    QAbstractScrollArea::keyPressEvent(pEvent);
}

void XAbstractTableView::wheelEvent(QWheelEvent *pEvent)
{
    QAbstractScrollArea::wheelEvent(pEvent);
}

bool XAbstractTableView::isOffsetValid(qint64 nOffset)
{
    Q_UNUSED(nOffset)

    return false;
}

bool XAbstractTableView::isEnd(qint64 nOffset)
{
    Q_UNUSED(nOffset)

    return false;
}

qint64 XAbstractTableView::cursorPositionToOffset(XAbstractTableView::CURSOR_POSITION cursorPosition)
{
    Q_UNUSED(cursorPosition)

    return 0;
}

void XAbstractTableView::updateData()
{

}

void XAbstractTableView::startPainting(QPainter *pPainter)
{
    Q_UNUSED(pPainter)
}

void XAbstractTableView::paintColumn(QPainter *pPainter, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight)
{
    Q_UNUSED(pPainter)
    Q_UNUSED(nColumn)
    Q_UNUSED(nLeft)
    Q_UNUSED(nTop)
    Q_UNUSED(nWidth)
    Q_UNUSED(nHeight)
}

void XAbstractTableView::paintCell(QPainter *pPainter, qint32 nRow, qint32 nColumn, qint32 nLeft, qint32 nTop, qint32 nWidth, qint32 nHeight)
{
    Q_UNUSED(pPainter)
    Q_UNUSED(nRow)
    Q_UNUSED(nColumn)
    Q_UNUSED(nLeft)
    Q_UNUSED(nTop)
    Q_UNUSED(nWidth)
    Q_UNUSED(nHeight)
}

void XAbstractTableView::endPainting(QPainter *pPainter)
{
    Q_UNUSED(pPainter)
}

bool XAbstractTableView::_goToOffset(qint64 nOffset)
{
    bool bResult=false;

    if(isOffsetValid(nOffset))
    {
        setScrollValue(nOffset);
        bResult=true;
    }

    return bResult;
}

void XAbstractTableView::contextMenu(const QPoint &pos)
{
    Q_UNUSED(pos)
}

qint64 XAbstractTableView::getScrollValue()
{
    return verticalScrollBar()->value();
}

void XAbstractTableView::setScrollValue(qint64 nOffset)
{
    verticalScrollBar()->setValue((qint32)nOffset);
}
