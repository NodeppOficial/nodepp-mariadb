#ifndef NODEPP_MARIADB
#define NODEPP_MARIADB

/*────────────────────────────────────────────────────────────────────────────*/

#include <mariadb/mysql.h>
#include <nodepp/nodepp.h>
#include <nodepp/url.h>

namespace nodepp { using sql_item_t = map_t<string_t,string_t>; }

namespace nodepp { class mariadb_t {
protected:

    struct NODE {
        MYSQL *fd = nullptr;
        int state = 1;
    };  ptr_t<NODE> obj;

    template< class T > void callback( T& cb ) const { 
        MYSQL_RES *res = mysql_store_result(obj->fd);
        sql_item_t arguments; array_t<string_t> col;

        if( res == NULL ) { //mysql_free_result( res );
            string_t message = mysql_error(obj->fd);
            process::error( message );
        }

        int num_fields = mysql_num_fields( res );
        MYSQL_ROW row  = mysql_fetch_row( res );

        for( int x=0; x<num_fields; x++ )
           { col.push( row[x] ); }

        while( (row=mysql_fetch_row(res)) ){
          for( int x=0; x<num_fields; x++ ){
               arguments[ col[x] ] = row[x] ? row[x] : "NULL"; 
        } cb ( arguments ); }

        mysql_free_result( res );
    }

public:
    
    virtual ~mariadb_t() noexcept {
        if( obj.count() > 1 || obj->fd == nullptr ){ return; }
        if( obj->state == 0 ){ return; } free();
    }
    
    /*─······································································─*/

    virtual void free() const noexcept {
        if( obj->fd == nullptr ){ return; }
        if( obj->state == 0 )   { return; }
        mysql_close( obj->fd );
        obj->state = 0; 
    }
    
    /*─······································································─*/
    
#ifdef NODEPP_SSL
    mariadb_t ( string_t uri, string_t name, ssl_t* ssl ) : obj( new NODE ) {
        
        obj->fd = mysql_init(NULL); if( obj->fd == nullptr )
          { process::error("Error: Can't Start MySQL"); }

        auto host = url::hostname( uri );
        auto user = url::user( uri );
        auto pass = url::pass( uri );
        auto port = url::port( uri );

        char* key = ssl->get_key_path().get();
        char* crt = ssl->get_crt_path().get(); mysql_ssl_set( obj->fd, key, crt, NULL , NULL, NULL );

        if( mysql_real_connect( obj->fd, host.get(), user.get(), pass.get(), name.get(), port, NULL, 0 ) == NULL ){
            string_t message = mysql_error( obj->fd ); process::error( message );
        }

    }
#endif
    
    /*─······································································─*/
    
    mariadb_t ( string_t uri, string_t name ) : obj( new NODE ) {
        
        obj->fd = mysql_init(NULL); if( obj->fd == nullptr )
          { process::error("Error: Can't Start MySQL"); }

        auto host = url::hostname( uri );
        auto user = url::user( uri );
        auto pass = url::pass( uri );
        auto port = url::port( uri );

        if( mysql_real_connect( obj->fd, host.get(), user.get(), pass.get(), name.get(), port, NULL, 0 ) == NULL ){
            string_t message = mysql_error( obj->fd ); process::error( message );
        }

    }
    
    /*─······································································─*/

    void exec( const string_t& cmd, const function_t<void,sql_item_t>& cb ) const {
        if( mysql_real_query( obj->fd, cmd.data(), cmd.size() ) != 0 ){
            string_t message = mysql_error( obj->fd );
            process::error( message );
        }   callback( cb );
    }

    array_t<sql_item_t> exec( const string_t& cmd ) const { array_t<sql_item_t> res;
        function_t<void,sql_item_t> cb = [&]( sql_item_t args ){ res.push( args ); };
        if( mysql_real_query( obj->fd, cmd.data(), cmd.size() ) != 0 ){
            string_t message = mysql_error( obj->fd );
            process::error( message );
        }   callback( cb ); return res;
    }

};}

/*────────────────────────────────────────────────────────────────────────────*/

#endif
