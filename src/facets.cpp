#include "stdafx.h"

#include "facets.h"
#include "facets_autoplaylist_client.h"

namespace httpc
{
namespace facets
{
	pfc::stringLite query[HTTPC_FACETS_COUNT];
	t_size facet_current;
	pfc::list_t<pfc::stringLite> filtered;
	pfc::stringLite selected[HTTPC_FACETS_COUNT];

	void fill_playlist()
	{
		if (!library_manager::get()->is_library_enabled()
			|| facet_current == 0)
			return;

		auto apm = autoplaylist_manager_v2::get();
		auto pm = playlist_manager::get();

		auto playlist = query_select_or_create_playlist(cfg.query.sendtodedicated, cfg.misc.query_playlist_name);

		if (playlist != pfc::infinite_size)
		{
			autoplaylist_client::ptr foo_apl_client_ptr = new service_impl_t<foo_autoplaylist_client>;

			apm->add_client(foo_apl_client_ptr, playlist, autoplaylist_flag_sort);
			
			httpc::should_update_playlist = true;
			httpc::query_playlist = playlist;
			httpc::should_update_playlists_list = true;
			httpc::playlist_page = 1;
		}
	}

	void filter(t_size which_facet)
	{
		query[0] = cfg.query.step1;
		query[1] = cfg.query.step2;
		query[2] = cfg.query.step3;

		if (!static_api_ptr_t<library_manager>()->is_library_enabled()
			|| which_facet >= HTTPC_FACETS_COUNT
			|| strlen(query[which_facet]) == 0)
		{
			filtered.remove_all();
			return;
		}
/*
		pfc::hires_timer timer;
		timer.start();
*/
		pfc::list_t<metadb_handle_ptr> library;
		pfc::stringLite facet, var_name;
		pfc::map_t<pfc::stringLite, boolean> items;
		service_ptr_t<titleformat_object> script_query;
		auto titleformat_compiler = titleformat_compiler::get();

		titleformat_compiler->compile_safe(script_query, query[which_facet]);

		library_manager::get()->get_all_items(library);

		auto lc = library.get_count();
		pfc::chain_list_v2_t<pfc::stringLite> field_values;

		if (which_facet == 0) // don't have to do extra search
			for (size_t i = 0; i < lc; ++i)
			{
				if (library.get_item(i)->format_title(NULL, facet, script_query, NULL))
				{
					if (facet.find_first(';') == pfc::infinite_size)
						items[facet] = true;
					else
					{
						field_values.remove_all();
						pfc::splitStringSimple_toList(field_values, ";", facet);

						for (auto walk = field_values.first(); walk.is_valid(); ++walk)
						{
							walk->skip_trailing_char();
							items[*walk] = true;
						}
					}
				}
			}
		else
		{
			t_size j;
			bool acceptable;

			pfc::list_t<service_ptr_t<titleformat_object>> scripts;
			scripts.set_count(which_facet+1);
			for (j = 0; j <= which_facet; ++j)
				titleformat_compiler->compile_safe(scripts[j],query[j]);

			for (size_t i = 0; i < lc; ++i)
			{
				acceptable = true;
				for (j = 0; j < which_facet && acceptable; ++j)
					if (library.get_item(i)->format_title(NULL, facet, scripts[j], NULL))
					{
						if ((facet.find_first(';') == pfc::infinite_size) && (facet != selected[j]))
							acceptable = false;
						else
						{
							field_values.remove_all();
							pfc::splitStringSimple_toList(field_values, ";", facet);
							acceptable = list_find(field_values, selected[j]);
						}
					}
					else
						acceptable = false;

				if (acceptable && library.get_item(i)->format_title(NULL, facet, script_query, NULL))
					if (facet.get_length() > 0)
					{
						if (facet.find_first(';') == pfc::infinite_size)
							items[facet] = true;
						else
						{
							field_values.remove_all();
							pfc::splitStringSimple_toList(field_values, ";", facet);

							for (auto walk = field_values.first(); walk.is_valid(); ++walk)
							{
								walk->skip_trailing_char();
								items[*walk] = true;
							}
						}
					}
			}

			for (j = 0; j <= which_facet; ++j)
				scripts[j].release();
		}

		filtered.remove_all();

		for (auto iter = items.first(); iter.is_valid(); ++iter)
			filtered.add_item(xml_friendly_string(pfc::stringLite(iter->m_key)));

		filtered.sort_t(sortfunc_natural);

		script_query.release();

		/*
			console::formatter() << "facet                     " << which_facet;
		console::formatter() << "library items             " << library.get_count();
		console::formatter() << "filtered items            " << filtered.get_count();
		console::formatter() << "filtered list in " << pfc::format_time_ex(timer.query(),6);
		timer.start();
		console::formatter() << "filtered list sorted in   " << pfc::format_time_ex(timer.query(),6);

		for (size_t i = 0; i < filtered.get_count(); ++i)
			console::info(filtered.get_item(i));
	*/
	};

	void prev_facet()
	{
		if (facet_current < HTTPC_FACETS_COUNT)
			selected[facet_current].reset();

		if (facet_current > 0)
		{
			--facet_current;
			selected[facet_current].reset();
		}

		filter(facet_current);
	}

	void next_facet(pfc::string_base &request)
	{
		if (facet_current > HTTPC_FACETS_COUNT - 1)
			facet_current = HTTPC_FACETS_COUNT - 1;

		selected[facet_current] = request;

		++facet_current;

		filter(facet_current);
	}

	bool list_find(pfc::chain_list_v2_t<pfc::stringLite> &list, pfc::stringLite &str)
	{
		for (auto walk = list.first(); walk.is_valid(); ++walk)
		{
			walk->skip_trailing_char();

			if (*walk == str)
				return true;
		}

		return false;
	}
}
}