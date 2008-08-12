#pragma once

#ifndef Q2DTRANSFERFUNCTION
#define Q2DTRANSFERFUNCTION

#include <QtGui/QWidget>
#include <IO/TransferFunction2D.h>

#define Q2DT_PAINT_NONE  0
#define Q2DT_PAINT_RED   1
#define Q2DT_PAINT_GREEN 2
#define Q2DT_PAINT_BLUE  4
#define Q2DT_PAINT_ALPHA 8
#define Q2DT_PAINT_UNDEF 16

class MasterController;

class Q2DTransferFunction : public QWidget
{
	Q_OBJECT

	public:
		Q2DTransferFunction(MasterController& masterController, QWidget *parent=0);
		virtual ~Q2DTransferFunction(void);
		void SetData(const Histogram2D* vHistrogram, TransferFunction2D* pTrans);
		void SetPaintmode(unsigned int iPaintmode) {if (iPaintmode < Q2DT_PAINT_UNDEF) m_iPaintmode = iPaintmode;};

		QSize minimumSizeHint() const;
		QSize sizeHint() const;

	public slots:
//		void SetActiveSwatch(const int iActiveSwatch);
//		void AddSwatch();
//		void DeleteCurrentSwatch();

		bool LoadFromFile(const QString& strFilename);
		bool SaveToFile(const QString& strFilename);

//	signals:
//		void ChangedActiveSwatch();
	
	protected:
		virtual void paintEvent(QPaintEvent *event);
		virtual void mouseMoveEvent(QMouseEvent *event);
		virtual void mousePressEvent(QMouseEvent *event);
		virtual void mouseReleaseEvent(QMouseEvent *event);
		virtual void changeEvent(QEvent * event);

	private:
		// states
		NormalizedHistogram2D	m_vHistrogram;
		TransferFunction2D*		m_pTrans;
		unsigned int m_iPaintmode;
		int m_iActiveSwatchIndex;
		MasterController& m_MasterController;

		// cached image of the backdrop
		bool		 m_bBackdropCacheUptodate;
		unsigned int m_iCachedHeight;
		unsigned int m_iCachedWidth;
		QPixmap*	 m_pBackdropCache;

		// border size, may be changed in the constructor 
		unsigned int m_iBorderSize;
		unsigned int m_iSwatchBorderSize;

		// colors, may be changed in the setcolor 
		QColor m_colorHistogram;
		QColor m_colorBack;
		QColor m_colorBorder;
		QColor m_colorSwatchBorder;
		QColor m_colorSwatchBorderCircle;
		QColor m_colorSwatchGradCircle;

		void SetColor(bool bIsEnabled);

		// mouse motion handling
		int m_iLastIndex;
		float m_fLastValue;

		// drawing routines
		void DrawBorder(QPainter& painter);
		void DrawHistogram(QPainter& painter, float fScale=1.0f);
		void DrawSwatches(QPainter& painter);

		// helper
		INTVECTOR2 Rel2Abs(FLOATVECTOR2 vfCoord);
};


#endif // Q2DTRANSFERFUNCTION
