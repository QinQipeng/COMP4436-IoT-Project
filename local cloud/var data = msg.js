var data = msg.payload;
var output = JSON.stringify(data);
msg.payload = output;
msg.payload = JSON.parse(msg.payload).uplink_message.frm_payload;
msg.payload = new Buffer(msg.payload, 'base64').toString('ascii');
return msg;