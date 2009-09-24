﻿
//         Copyright Eóin O'Callaghan 2006 - 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "stdAfx.hpp"

#include "../HaliteWindow.hpp"
#include "../HaliteListView.hpp"
#include "../global/logger.hpp"

#include "Peers.hpp"

bool PeerListView::sort_list_comparison(std::wstring l, std::wstring r, size_t index, bool ascending)
{
	hal::peer_details_vec::optional_type pdl = peer_details_.find_peer(l);
	hal::peer_details_vec::optional_type pdr = peer_details_.find_peer(r);

	if (pdl && pdr) 
		return hal::hal_details_ptr_compare(pdl, pdr, index, ascending);
	else
		return false;
}

LRESULT PeerListView::OnGetDispInfo(int, LPNMHDR pnmh, BOOL&)
{	
	NMLVDISPINFO* pdi = (NMLVDISPINFO*)pnmh;

	HAL_DEV_SORT_MSG(hal::wform(L"OnGetDispInfo index = %1% size = %2%") % pdi->item.iItem % peer_details_.size());

	hal::try_update_lock<listClass> lock(*this);
	if (lock && pdi->item.iItem >= 0 && peer_details_.size() >= numeric_cast<unsigned>(pdi->item.iItem)) 
	{	

	hal::peer_details_vec::optional_type pd = peer_details_.find_peer(key_from_index(pdi->item.iItem));

	if (pd && pdi->item.mask & LVIF_TEXT)
	{
		wstring str = pd->to_wstring(pdi->item.iSubItem);
		
		size_t len = str.copy(pdi->item.pszText, min(pdi->item.cchTextMax - 1, static_cast<int>(str.size())));
		pdi->item.pszText[len] = '\0';
	}

	}
	
	return 0;
}

void PeerListView::uiUpdate(const hal::torrent_details_manager& tD)
{
	hal::try_update_lock<listClass> lock(*this);
	if (lock) 
	{		
		selection_from_listview();
		
		peer_details_.clear();
		
		foreach (const std::wstring filename, tD.selected_names())
		{
			const hal::torrent_details_ptr t = tD.get(filename);
			if (t)
			{
			std::copy(t->get_peer_details().begin(), t->get_peer_details().end(), 
				std::inserter(peer_details_, peer_details_.begin()));
			}
		}
		
		std::set<std::wstring> ip_set;
		foreach (hal::peer_detail& pd,  peer_details_)
			ip_set.insert(pd.ip_address);
		
		erase_based_on_set(ip_set, true);

		if (IsSortOnce() || AutoSort())
		{
			if (GetSecondarySortColumn() != -1)
			{
				int index = GetColumnSortType(GetSecondarySortColumn());					
				if (index > WTL::LVCOLSORT_LAST)
					sort(index - (WTL::LVCOLSORT_LAST+1+hal::peer_detail::ip_address_e), IsSecondarySortDescending());
			}

			if (GetSortColumn() != -1)
			{		
				int index = GetColumnSortType(GetSortColumn());				
				if (index > WTL::LVCOLSORT_LAST)
					sort(index - (WTL::LVCOLSORT_LAST+1+hal::peer_detail::ip_address_e), IsSortDescending());
			}
		}
		
		set_keys(ip_set);		
		InvalidateRect(NULL,true);
	}
}

LRESULT PeerListView::OnSortChanged(int, LPNMHDR pnmh, BOOL&)
{
	halite_window_.issueUiUpdate();
	
	return 0;
}

void AdvPeerDialog::uiUpdate(const hal::torrent_details_manager& tD)
{
	peerList_.uiUpdate(tD);
}

LRESULT AdvPeerDialog::OnInitDialog(HWND, LPARAM)
{	
	peerList_.SubclassWindow(GetDlgItem(HAL_PEERLIST));
	
	resizeClass::DlgResize_Init(false, true, WS_CLIPCHILDREN);	
	
	return 0;
}

void AdvPeerDialog::OnClose()
{
	if(::IsWindow(m_hWnd)) 
	{
		::DestroyWindow(m_hWnd);
	}
}
