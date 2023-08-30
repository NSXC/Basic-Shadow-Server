const net = require('net');
const axios = require('axios');
const blockedIPs = new Map();
const blockThreshold = 20;
const blockDuration = 300;
const REDIRECT_RESPONSE = 'IP BANNED';
//DDOS STUFF
const server = net.createServer(socket => {
  const clientIP = socket.remoteAddress;
  if (blockedIPs.has(clientIP)) {
    console.log(`Blocked IP: ${clientIP}`);
    socket.write(REDIRECT_RESPONSE);
    socket.end();
    return;
  }
  let messageCount = 0;
  let lastMessageTime = Date.now();
  let highRateDetected = false;
  socket.on('data', async data => {
    const currentTime = Date.now();
    const elapsedTime = currentTime - lastMessageTime;
    if (elapsedTime <= 1000) {
      messageCount++;
      lastMessageTime = currentTime;
      if (messageCount > blockThreshold && !highRateDetected) {
        highRateDetected = true;
        console.log('High message rate anomaly detected!');
        if (!blockedIPs.has(clientIP)) {
          blockedIPs.set(clientIP, true);
          console.log(`Blocking IP: ${clientIP}`);
          setTimeout(() => {
            blockedIPs.delete(clientIP);
            highRateDetected = false;
            console.log(`Unblocking IP: ${clientIP}`);
          }, blockDuration * 1000);
        }
      }
    } else {
      messageCount = 1;
      lastMessageTime = currentTime;
    }




//SERVER SUTFF
    try {
      const json = JSON.parse(data.toString());

      const messageId = Object.keys(json)[0];
      const message = json[messageId]?.data || '';
      const topic = json[messageId]?.topic || '';
      if(topic == "message"){
        if (message === 'ping') {
          socket.write('pong');

        }  
      }else if (topic == "verify"){
        const parts = message.split("=>");
        if (parts.length === 2) {
          const cookie = parts[0];
          const gameid = parts[1];
          const encodedRbxcookie = typeof cookie === 'string' ? encodeURIComponent(cookie) : '';
          const userinfo = await fetch(`http://localhost:8000/userinfo/${encodedRbxcookie}`)
          const jsonString = await userinfo.json();
          if(jsonString.Status == 400){
            socket.write("Bad Cookie")
          }else{
            let cref = jsonString.Token
            const session = axios.create({
              withCredentials: true
            });
            
            session.defaults.headers['Cookie'] = `.ROBLOSECURITY=${cookie}`;
            
            const headers = {
              'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:109.0) Gecko/20100101 Firefox/116.0',
              'Accept': '*/*',
              'Accept-Language': 'en-US,en;q=0.5',
              'Accept-Encoding': 'gzip, deflate, br',
              'X-CSRF-TOKEN':  `${cref}`,
              'Origin': 'https://www.roblox.com',
              'Referer': 'https://www.roblox.com/',
              'Sec-Fetch-Dest': 'empty',
              'Sec-Fetch-Mode': 'cors',
              'Sec-Fetch-Site': 'same-site',
              'TE': 'trailers',
            };
            const data = {};

            session.post('https://auth.roblox.com/v1/authentication-ticket/', data, { headers })
              .then(response => {
                const rbxAuthToken = response.headers['rbx-authentication-ticket'];
                socket.write(rbxAuthToken)

              })
              .catch(error => {
                console.error(error);
              });
          }
        } else {
          socket.write("Invalid input format.");
        }
      }
    } catch (error) {
      socket.write('Error');
    }
  });

  socket.on('end', () => {
    console.log('Client disconnected');
  });

  socket.on('error', error => {
    console.error('Socket Error:', error.message);
    socket.destroy();
  });
});

const PORT = 238;
server.listen(PORT, () => {
  console.log(`Server listening on port ${PORT}`);
});
