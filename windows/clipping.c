/*
 * Window clipping functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <stdio.h>

#include "windows.h"
#include "win.h"
#include "message.h"


/***********************************************************************
 *           InvalidateRgn   (USER.126)
 */
void InvalidateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    HRGN newRgn;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return;
    
    if (!hrgn)
    {
	newRgn = CreateRectRgn(0, 0, 
		   wndPtr->rectClient.right-wndPtr->rectClient.left,
		   wndPtr->rectClient.bottom-wndPtr->rectClient.top );
    }
    else 
    {
	if (!(newRgn = CreateRectRgn( 0, 0, 0, 0 ))) return;
	if (!wndPtr->hrgnUpdate) CombineRgn( newRgn, hrgn, 0, RGN_COPY );
	else CombineRgn( newRgn, wndPtr->hrgnUpdate, hrgn, RGN_OR );
    }
    if (wndPtr->hrgnUpdate) DeleteObject( wndPtr->hrgnUpdate );
    else MSG_IncPaintCount( wndPtr->hmemTaskQ );
    wndPtr->hrgnUpdate = newRgn;
    if (erase) wndPtr->flags |= WIN_ERASE_UPDATERGN;

      /* Invalidate the children overlapping the region */

    if (wndPtr->dwStyle & WS_CLIPCHILDREN) return;
    for (hwnd = wndPtr->hwndChild; (hwnd); hwnd = wndPtr->hwndNext)
    {
	if (!(wndPtr = WIN_FindWndPtr( hwnd ))) break;
	if (hrgn && !RectInRegion( hrgn, &wndPtr->rectWindow )) continue;
	InvalidateRgn( hwnd, hrgn, erase );
    }
}


/***********************************************************************
 *           InvalidateRect   (USER.125)
 */
void InvalidateRect( HWND hwnd, LPRECT rect, BOOL erase )
{
    HRGN hrgn = 0;

    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return;

#ifdef DEBUG_WIN
    if (rect) printf( "InvalidateRect: %d %d,%d-%d,%d\n", hwnd,
		     rect->left, rect->top, rect->right, rect->bottom );
    else printf( "InvalidateRect: %d NULL\n", hwnd );
#endif
    if (rect) hrgn = CreateRectRgnIndirect( rect );
    InvalidateRgn( hwnd, hrgn, erase );
    if (hrgn) DeleteObject( hrgn );
}


/***********************************************************************
 *           ValidateRgn   (USER.128)
 */
void ValidateRgn( HWND hwnd, HRGN hrgn )
{
    HRGN newRgn;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return;

    if (!wndPtr->hrgnUpdate) return;
    if (!hrgn) newRgn = 0;
    else
    {
	if (!(newRgn = CreateRectRgn( 0, 0, 0, 0 ))) return;
	if (CombineRgn( newRgn, wndPtr->hrgnUpdate, hrgn, RGN_DIFF ) == NULLREGION)
	{
	    DeleteObject( newRgn );
	    newRgn = 0;
	}
    }
    DeleteObject( wndPtr->hrgnUpdate );
    wndPtr->hrgnUpdate = newRgn;
    if (!wndPtr->hrgnUpdate) wndPtr->flags &= ~WIN_ERASE_UPDATERGN;
}


/***********************************************************************
 *           ValidateRect   (USER.127)
 */
void ValidateRect( HWND hwnd, LPRECT rect )
{
    HRGN hrgn = 0;

    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return;

    if (rect) hrgn = CreateRectRgnIndirect( rect );
    ValidateRgn( hwnd, hrgn );
    if (hrgn) DeleteObject( hrgn );
}


/***********************************************************************
 *           GetUpdateRect   (USER.190)
 */
BOOL GetUpdateRect( HWND hwnd, LPRECT rect, BOOL erase )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return FALSE;

    if (rect)
    {
	if (wndPtr->hrgnUpdate)
	{
	    HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );
	    if (GetUpdateRgn( hwnd, hrgn, erase ) == ERROR) return FALSE;
	    GetRgnBox( hrgn, rect );
	    DeleteObject( hrgn );
	}
	else SetRectEmpty( rect );
    }
    return (wndPtr->hrgnUpdate != 0);
}


/***********************************************************************
 *           GetUpdateRgn   (USER.237)
 */
int GetUpdateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    HRGN hrgnClip;
    int retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return ERROR;

    if (!wndPtr->hrgnUpdate)
    {
	if (!(hrgnClip = CreateRectRgn( 0, 0, 0, 0 ))) return ERROR;
	retval = CombineRgn( hrgn, hrgnClip, 0, RGN_COPY );
    }
    else
    {
	hrgnClip = CreateRectRgn( 0, 0,
			   wndPtr->rectClient.right-wndPtr->rectClient.left,
			   wndPtr->rectClient.bottom-wndPtr->rectClient.top );
	if (!hrgnClip) return ERROR;
	retval = CombineRgn( hrgn, wndPtr->hrgnUpdate, hrgnClip, RGN_AND );
	if (erase)
	{
	    HDC hdc = GetDCEx( hwnd, wndPtr->hrgnUpdate,
			      DCX_INTERSECTRGN | DCX_USESTYLE );
	    if (hdc)
	    {
		SendMessage( hwnd, WM_ERASEBKGND, hdc, 0 );
		ReleaseDC( hwnd, hdc );
	    }
	}	
    }
    DeleteObject( hrgnClip );
    return retval;
}
