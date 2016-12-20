#pragma once


// CPxListBox

class CPxListBox : public CListBox
{
	DECLARE_DYNAMIC(CPxListBox)

public:
	CPxListBox();
	virtual ~CPxListBox();

protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT /*lpMeasureItemStruct*/);
};


