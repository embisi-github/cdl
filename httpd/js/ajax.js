function Ajax()
{
    this.reqs = null;
    this.url = null;
    this.method = 'GET';
    this.async = true;
    this.status = null;
    this.statusText = '';
    this.postData = null;
    this.readyState = null;
    this.responseText = null;
    this.responseXML = null;
    this.handleResp = null;
    this.responseFormat = 'text'; // 'text', 'xml', or 'object'
    this.mimeType = null;

    this.init = function()
        {
            if (!this.req)
            {
                this.req = new XMLHttpRequest();
            }
            return this.req;
        };

    this.handleErr = function( err )
        {
            alert('An error occurred, but the error message cannot be '
                  + 'displayed. This is probably because of your browser\'s '
                  + 'pop-up blocker.\n'
                  + 'Please allow pop-ups from this web site if you want to '
                  + 'see the full error messages.\n'
                  + '\n'
                  + 'URL: ' + this.url + '\n'
                  + 'Status Code: ' + this.req.status + '\n'
                  + 'Status Code: ' + this.req.statusText + '\n'
                  + 'Response: ' + this.req.responseText);
        };
    this.doReq = function()
        {
            if (!this.init())
            {
                alert('Could not create XMLHttpRequest object.');
                return;
            }
            var this_ajax = this; // Fix loss-of-scope in inner function
            function req_handler()
            {
                if (this.readyState == 4) {
                    if (this.status >= 200 && this.status <= 299)
                    {
                        if (this_ajax.expected_format == 'xml')
                        {
                            this_ajax.handleResp(this.responseXML);
                        }
                        else
                        {
                            this_ajax.handleResp(this.responseText);
                        }
                    }
                    else
                    {
                        this_ajax.handleErr(this.responseText);
                    }
                }
            };
            this.req.onreadystatechange = req_handler;
            this.req.open(this.method, this.url, this.async);
            this.req.send(this.postData);
        };


    this.setHandlerErr = function(funcRef) {
        this.handleErr = funcRef;  
    }

    // The following should be called on a page 'stop' being clicked
    this.abort = function() {
        if (this.req) {
            this.req.onreadystatechange = function() { };
            this.req.abort();
            this.req = null;
        }
    };

    this.doGet = function(url, hand, format)
        {
            this.url = url;
            this.handleResp = hand;
            this.expected_format = format;
            this.responseFormat = format || 'text';
            this.doReq();
        };
};


