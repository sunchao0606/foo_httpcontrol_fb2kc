#include "stdafx.h"
#include "facets_autoplaylist_client.h"

namespace httpc
{
namespace facets
{
	GUID foo_autoplaylist_client::get_guid() {
		static const GUID apl = { 0x97e0fef2, 0x7c8b, 0x4d7e, { 0xa0, 0x72, 0x5b, 0x25, 0x47, 0xe9, 0x9e, 0xa1 } };
		return apl;
	}

	void foo_autoplaylist_client::filter(metadb_handle_list_cref data, bool* out) {
		t_size i, j, l;
		bool acceptable;
		pfc::string8 facet;

		pfc::list_t<service_ptr_t<titleformat_object>> scripts;
		scripts.set_count(facet_current + 1);
		for (j = 0; j <= facet_current; ++j)
			static_api_ptr_t<titleformat_compiler>()->compile_safe(scripts[j], query[j]);

		pfc::chain_list_v2_t<pfc::string8> field_values;

		l = data.get_count();
		for (i = 0; i < l; ++i)
		{
			acceptable = true;

			for (t_size j = 0; j < facet_current && selected[j].get_length(); ++j)
				if (data.get_item(i)->format_title(NULL, facet, scripts[j], NULL))
				{
					if (facet.find_first(';') == pfc::infinite_size && facet != selected[j])
					{
						acceptable = false;
						break;
					}
					else
					{
						field_values.remove_all();
						pfc::splitStringSimple_toList(field_values, ';', facet);

						if (!list_find(field_values, selected[j]))
						{
							acceptable = false;
							break;
						}
					}
				}
				else
				{
					acceptable = false;
					break;
				}
			out[i] = acceptable;
		}

		for (j = 0; j <= facet_current; ++j)
			scripts[j].release();
	};

	bool foo_autoplaylist_client::sort(metadb_handle_list_cref p_items, t_size* p_orderbuffer)
	{
		metadb_handle_list_helper::sort_by_format_get_order(p_items, p_orderbuffer, cfg.query.sortpattern, NULL);
		// todo! hack, I suppose
		return false;
	};
}
}