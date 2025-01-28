#ifndef ___FACETS_H___
#define ___FACETS_H___

#define HTTPC_FACETS_COUNT 3

namespace httpc
{
namespace facets
{
	extern pfc::stringLite query[HTTPC_FACETS_COUNT];
	extern t_size facet_current;
	extern pfc::list_t < pfc::stringLite > filtered;
	extern pfc::stringLite selected[HTTPC_FACETS_COUNT];

	static int sortfunc_natural(pfc::string_base& p1, pfc::string_base& p2) { return compare_natural_utf8(p1, p2); }

	extern void filter(t_size which_facet);
	extern void fill_playlist();
	extern void prev_facet();
	extern void next_facet(pfc::string_base &request);
	extern bool list_find(pfc::chain_list_v2_t<pfc::stringLite> &list, pfc::stringLite &str);
}
}

#endif //___FACETS_H___