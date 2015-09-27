#include <stdio.h>
#include "apr_hash.h"
#include "ap_config.h"
#include "ap_provider.h"
#include "errno.h"
#include "httpd.h"
#include "http_core.h"
#include "http_config.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"

static void stdoutreader_hooks( apr_pool_t* inpPool );
static int stdoutreader_handler( request_rec* inpRequest );

typedef struct {
    const char* working_dir;
    const char* recording_dir;
} stdoutreader_config;

static stdoutreader_config config;

// ============================================================================
// Methods for reading configuration parameters
// ============================================================================

const char* stdoutreader_set_workingdir(cmd_parms* cmd, void* cfg, const char* arg) {
    config.working_dir = arg;
    return NULL;
}

const char* stdoutreader_set_recordingdir(cmd_parms* cmd, void* cfg, const char* arg) {
    config.recording_dir = arg;
    return NULL;
}

// ============================================================================
// Directives to read configuration parameters
// ============================================================================

static const command_rec stdoutreader_directives[] =
{
    AP_INIT_TAKE1( "workingDir", stdoutreader_set_workingdir, NULL, RSRC_CONF, "Working directory" ),
    AP_INIT_TAKE1( "recordingDir", stdoutreader_set_recordingdir, NULL, RSRC_CONF, "Recording directory" ),
    { NULL }
};

// ============================================================================
// Module definition
// ============================================================================

module AP_MODULE_DECLARE_DATA stdoutreader_module =
{
    STANDARD20_MODULE_STUFF,
    NULL,
    NULL,
    NULL,
    NULL,
    stdoutreader_directives,
    stdoutreader_hooks
};

// ============================================================================
// Module handler function
// ============================================================================

int stdoutreader_handler( request_rec* inpRequest )
{
    if ( !inpRequest->handler || strcmp( inpRequest->handler, "stdoutreader-handler" ))
    {
        return DECLINED;
    }

    const char* request_method = inpRequest->method;
    const char* request_uri = inpRequest->unparsed_uri;
    const char* protocol = inpRequest->protocol;
    const char* http_host = inpRequest->hostname;
    const char* user_agent = apr_table_get( inpRequest->headers_in, "User-Agent" );

    setenv( "MAHIMAHI_CHDIR", config.working_dir, TRUE );
    setenv( "MAHIMAHI_RECORD_PATH", config.recording_dir, TRUE );
    setenv( "REQUEST_METHOD", request_method, TRUE );
    setenv( "REQUEST_URI", request_uri, TRUE );
    setenv( "SERVER_PROTOCOL", protocol, TRUE );
    setenv( "HTTP_HOST", http_host, TRUE );
    if ( user_agent != NULL ) {
        setenv( "HTTP_USER_AGENT", user_agent, TRUE );
    }

    FILE* fp = popen( "mm-replayserver", "r" );
    if ( fp == NULL ) {
        // "Error encountered while running script"
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    char line[HUGE_STRING_LEN];

    apr_file_t* file;
    apr_os_file_t os_file = (apr_os_file_t) fileno( fp );
    apr_os_pipe_put( &file, &os_file, inpRequest->pool );
    ap_scan_script_header_err( inpRequest, file, line );

    int num_bytes_read;
    do {
        num_bytes_read = fread( line, sizeof(char), HUGE_STRING_LEN, fp );
        int num_bytes_left = num_bytes_read;
        while ( num_bytes_left > 0 ) {
            int offset = num_bytes_read - num_bytes_left;
            int num_bytes_written = ap_rwrite( line + offset, num_bytes_left, inpRequest );
            if ( num_bytes_written == -1 ) {
                // "Error encountered while writing"
                return HTTP_INTERNAL_SERVER_ERROR;
            }
            num_bytes_left -= num_bytes_written;
        }
    } while ( num_bytes_read == HUGE_STRING_LEN );

    pclose( fp );

    return OK;
}

// ============================================================================
// Definition of hook for handler
// ============================================================================

void stdoutreader_hooks( apr_pool_t* inpPool )
{
    ap_hook_handler( stdoutreader_handler, NULL, NULL, APR_HOOK_LAST );
}

