// pxListBox.cpp : 实现文件
//

#include "stdafx.h"
#include "pxListBox.h"

/*

References:
http://www.cnblogs.com/findumars/p/6005599.html

*/

// CPxListBox

IMPLEMENT_DYNAMIC(CPxListBox, CListBox)

CPxListBox::CPxListBox()
{

}

CPxListBox::~CPxListBox()
{
}


BEGIN_MESSAGE_MAP(CPxListBox, CListBox)
END_MESSAGE_MAP()

void CPxListBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	// TODO: Add your code to draw the specified item  
	ASSERT(lpDrawItemStruct->CtlType == ODT_LISTBOX);  
	LPCTSTR lpszText = (LPCTSTR) lpDrawItemStruct->itemData;  
	ASSERT(lpszText != NULL);  
	CDC dc;  

	dc.Attach(lpDrawItemStruct->hDC);  

	// Save these value to restore them when done drawing.  
	COLORREF crOldTextColor = dc.GetTextColor();  
	COLORREF crOldBkColor = dc.GetBkColor();  

	// If this item is selected, set the background color   
	// and the text color to appropriate values. Also, erase  
	// rect by filling it with the background color.  
	if ((lpDrawItemStruct->itemAction | ODA_SELECT) &&  
		(lpDrawItemStruct->itemState & ODS_SELECTED))  
	{  
		dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));  
		dc.SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));  
		dc.FillSolidRect(&lpDrawItemStruct->rcItem,   
			::GetSysColor(COLOR_HIGHLIGHT));  
	}  
	else  
	{  
		if(lpDrawItemStruct->itemID%2)  
			dc.FillSolidRect(&lpDrawItemStruct->rcItem, RGB(128,128,128));  
		else  
			dc.FillSolidRect(&lpDrawItemStruct->rcItem, RGB(255,128,255));  
	}  

	// If this item has the focus, draw a red frame around the  
	// item's rect.  
	if ((lpDrawItemStruct->itemAction | ODA_FOCUS) &&  
		(lpDrawItemStruct->itemState & ODS_FOCUS))  
	{  
		CBrush br(RGB(0, 0, 128));  
		dc.FrameRect(&lpDrawItemStruct->rcItem, &br);  
	}  
	lpDrawItemStruct->rcItem.left += 5;  
	// Draw the text.  
	dc.DrawText(  
		lpszText,  
		strlen(lpszText),  
		&lpDrawItemStruct->rcItem,  
		DT_WORDBREAK);  

	// Reset the background color and the text color back to their  
	// original values.  
	dc.SetTextColor(crOldTextColor);  
	dc.SetBkColor(crOldBkColor);  

	dc.Detach(); 
}

// 重写MeasureItem虚函数  
void CPxListBox::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	ASSERT(lpMeasureItemStruct->CtlType == ODT_LISTBOX);  
	LPCTSTR lpszText = (LPCTSTR) lpMeasureItemStruct->itemData;  
	ASSERT(lpszText != NULL);  
	CRect rect;  
	GetItemRect(lpMeasureItemStruct->itemID, &rect);  

	CDC* pDC = GetDC();   
	lpMeasureItemStruct->itemHeight = pDC->DrawText(lpszText, -1, rect, DT_WORDBREAK | DT_CALCRECT);   
	ReleaseDC(pDC);  
}
