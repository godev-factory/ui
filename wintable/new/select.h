// 13 december 2014

// damn winsock
static void doselect(struct table *t, intptr_t row, intptr_t column)
{
	RECT r, client;
	intptr_t oldrow;
	LONG width, height;
	struct rowcol rc;
	BOOL dovscroll;
	intptr_t i;
	intptr_t xpos;
	LONG clientWidth;

	oldrow = t->selectedRow;
	t->selectedRow = row;
	t->selectedColumn = column;

	if (GetClientRect(t->hwnd, &client) == 0)
		panic("error getting Table client rect in doselect()");
	client.top += t->headerHeight;
	height = rowht(t);

	// first vertically scroll to the new row to make it fully visible (or as visible as possible)
	if (t->selectedRow < t->vscrollpos)
		vscrollto(t, t->selectedRow);
	else {
		rc.row = t->selectedRow;
		rc.column = t->selectedColumn;
		// first assume entirely outside the client area
		dovscroll = TRUE;
		if (rowColumnToClientRect(t, rc, &r))
			// partially outside the client area?
			if (r.bottom <= client.bottom)		// <= here since we are comparing bottoms (which are the first pixels outside the rectangle)
				dovscroll = FALSE;
		if (dovscroll)
			vscrollto(t, t->selectedRow - t->vpagesize + 1);		// + 1 because apparently just t->selectedRow - t->vpagesize results in no scrolling (t->selectedRow - t->vpagesize == t->vscrollpos)...
	}

	// now see if the cell we want is to the left of offscreen, in which case scroll to its x-position
	xpos = 0;
	for (i = 0; i < t->selectedColumn; i++)
		xpos += columnWidth(t, i);
	if (xpos < t->hscrollpos)
		hscrollto(t, xpos);
	else {
		// if the full cell is not visible, scroll to the right just enough to make it fully visible (or as visible as possible)
		width = columnWidth(t, t->selectedColumn);
		clientWidth = client.right - client.left;
		if (xpos + width > t->hscrollpos + clientWidth)			// > because both sides deal with the first pixel outside
			// if the column is too wide, then just make it occupy the whole visible area (left-aligned)
			if (width > clientWidth)			// TODO >= ?
				hscrollto(t, xpos);
			else
				// TODO this formula is wrong
				hscrollby(t, clientWidth - width);
	}

	// now redraw the old and new /rows/
	// we do this after scrolling so the rectangles to be invalidated make sense
	r.left = client.left;
	r.right = client.right;
	if (oldrow != -1 && oldrow >= t->vscrollpos) {
		r.top = client.top + ((oldrow - t->vscrollpos) * height);
		r.bottom = r.top + height;
		if (InvalidateRect(t->hwnd, &r, TRUE) == 0)
			panic("error queueing previously selected row for redraw in doselect()");
	}
	// t->selectedRow must be visible by this point; we scrolled to it
	if (t->selectedRow != -1 && t->selectedRow != oldrow) {
		r.top = client.top + ((t->selectedRow - t->vscrollpos) * height);
		r.bottom = r.top + height;
		if (InvalidateRect(t->hwnd, &r, TRUE) == 0)
			panic("error queueing newly selected row for redraw in doselect()");
	}
}

// TODO which WM_xBUTTONDOWNs?
HANDLER(mouseDownSelectHandler)
{
	struct rowcol rc;

	rc = lParamToRowColumn(t, lParam);
	// don't check if lParamToRowColumn() returned row -1 or column -1; we want deselection behavior
	doselect(t, rc.row, rc.column);
	*lResult = 0;
	return TRUE;
}