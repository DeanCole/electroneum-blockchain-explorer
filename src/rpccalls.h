//
// Created by mwo on 13/04/16.
//


#ifndef CROWETN_RPCCALLS_H
#define CROWETN_RPCCALLS_H

#include "electroneum_headers.h"

#include <mutex>




namespace
{

// can be used to check if given class/struct exist
// from: https://stackoverflow.com/a/10722840/248823
template <typename T>
struct has_destructor
{
    // has destructor
    template <typename A>
    static std::true_type test(decltype(declval<A>().~A()) *)
    {
        return std::true_type();
    }

    // no constructor
    template <typename A>
    static std::false_type test(...)
    {
        return std::false_type();
    }

    /* This will be either `std::true_type` or `std::false_type` */
    typedef decltype(test<T>(0)) type;

    static const bool value = type::value;
};

}


namespace cryptonote
{
// declare struct in electroneum's cryptonote namespace.
// electroneum should provide definition for this,
// but we need to have it declared as we are going to
// check if its definition exist or not. depending on this
// we decide what gets to be defined as
// get_alt_blocks(vector<string>& alt_blocks_hashes);
struct COMMAND_RPC_GET_ALT_BLOCKS_HASHES;
}

namespace electroneumeg
{

using namespace cryptonote;
using namespace crypto;
using namespace std;



class rpccalls
{
    string deamon_url ;
    uint64_t timeout_time;

    std::chrono::milliseconds timeout_time_ms;

    epee::net_utils::http::url_content url;

    epee::net_utils::http::http_simple_client m_http_client;
    std::mutex m_daemon_rpc_mutex;

    string port;

public:

    rpccalls(string _deamon_url = "http:://127.0.0.1:26968",
             uint64_t _timeout = 200000);

    bool
    connect_to_electroneum_deamon();

    uint64_t
    get_current_height();

    bool
    get_mempool(vector<tx_info>& mempool_txs);

    bool
    commit_tx(tools::wallet2::pending_tx& ptx, string& error_msg);

    bool
    get_network_info(COMMAND_RPC_GET_INFO::response& info);

    bool
    get_dynamic_per_kb_fee_estimate(
            uint64_t grace_blocks,
            uint64_t& fee,
            string& error_msg);


    /**
     * This must be in the header for now, as it will be tempalte function
     *
     * @param alt_blocks_hashes
     * @return bool
     */
    template<typename T = COMMAND_RPC_GET_ALT_BLOCKS_HASHES>
    typename enable_if<has_destructor<T>::value, bool>::type
    get_alt_blocks(vector<string>& alt_blocks_hashes)
    {
        // definition of COMMAND_RPC_GET_ALT_BLOCKS_HASHES exist
        // so perform rpc call to get this information

        bool r {false};

        typename T::request req;
        typename T::response resp;

        {
            std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

            if (!connect_to_electroneum_deamon())
            {
                cerr << "get_alt_blocks: not connected to deamon" << endl;
                return false;
            }

            r = epee::net_utils::invoke_http_json("/get_alt_blocks_hashes",
                                                  req, resp,
                                                  m_http_client);
        }

        string err;

        if (r)
        {
            if (resp.status == CORE_RPC_STATUS_BUSY)
            {
                err = "daemon is busy. Please try again later.";
            }
            else if (resp.status != CORE_RPC_STATUS_OK)
            {
                err = "daemon rpc failed. Please try again later.";
            }

            if (!err.empty())
            {
                cerr << "Error connecting to Electroneum deamon due to "
                     << err << endl;
                return false;
            }
        }
        else
        {
            cerr << "Error connecting to Electroneum deamon at "
                 << deamon_url << endl;
            return false;
        }

        alt_blocks_hashes = resp.blks_hashes;

        return true;
    }

    template<typename T = COMMAND_RPC_GET_ALT_BLOCKS_HASHES>
    typename enable_if<!has_destructor<T>::value, bool>::type
    get_alt_blocks(vector<string>& alt_blocks_hashes)
    {
        cerr << "COMMAND_RPC_GET_ALT_BLOCKS_HASHES does not exist!" << endl;
        // definition of COMMAND_RPC_GET_ALT_BLOCKS_HASHES does NOT exist
        // so dont do anything
        return false;
    }

    bool
    get_block(string const& blk_hash, block& blk, string& error_msg);

};


}



#endif //CROWETN_RPCCALLS_H
