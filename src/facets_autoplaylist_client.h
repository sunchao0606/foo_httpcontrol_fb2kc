#ifndef ___FACETS_AUTOPLAYLIST_CLIENT_H___
#define ___FACETS_AUTOPLAYLIST_CLIENT_H___

#include "facets.h"

namespace httpc
{
namespace facets
{
class foo_autoplaylist_client : public autoplaylist_client {
public:
	foo_autoplaylist_client() {}

	GUID get_guid();

	void filter(metadb_handle_list_cref data, bool* out);

	bool sort(metadb_handle_list_cref p_items, t_size* p_orderbuffer);

	//! Retrieves your configuration data to be used later when re-instantiating your autoplaylist_client after a restart.
	void get_configuration(stream_writer* p_stream, abort_callback& p_abort) { }

	void show_ui(t_size p_source_playlist) { }
};

}
}

#endif //___FACETS_AUTOPLAYLIST_CLIENT___