const fetch = require("node-fetch");    //In the Beehavior project we use NodeJS Fetch API extension
                                        //to get node-fetch run: "npm install node-fetch@2"
class NRFCloudAPI {
	constructor(token) {                //JSON WEB Token, found on nRFCloud account page
		this.accessToken = token;
	}

	async request(endpoint, { body, method = 'POST' } = {}) {
		const response = await fetch(`https://api.nrfcloud.com/v1${endpoint}`, Object.assign({
			method,
			mode: 'cors',
			headers: Object.assign({
				Accept: 'application/json',
				Authorization: `Bearer ${this.accessToken}`,
			}, body ? {
				'Content-Type': 'application/json'
			} : undefined),
		}, body ? { body } : undefined));
		if (response.headers.get('content-type') === 'application/json' &&
			response.headers.get('content-length') !== '0') {
			return response.json();
		}
		return response.text();
	}

	get(endpoint) {
		return this.request(endpoint, {
			method: 'GET'
		});
	}
	
	// Get messages: https://docs.api.nrfcloud.com/api/api-rest.html#nrf-cloud-device-rest-api-messages
	getMessages(deviceId) {
		const end = new Date();
		const start = this.getMessages_start || new Date(end - 100000);         //If getMessages_start is not defined, then start will be the current time since epoch - 100 seconds
		this.getMessages_start = new Date();
		const devIdsParam = deviceId ? `&deviceIdentifiers=${deviceId}` : '';   //If deviceId is not an empty string, then devIdsParam is not an empty string.
		return this.get(`/messages?inclusiveStart=${start.toISOString()}&exclusiveEnd=${end.toISOString()}${devIdsParam}`);
	}
}

const nRFCloudConnection = new NRFCloudAPI("<API-key>");
const cycle_time = 5000 //The time for each cycle, here it is 5 seconds

let requestInterval = setInterval(async() => {
        const cloudMessageObject = await nRFCloudConnection.getMessages('');
        // Do something with the items
        for(let objectIndex = 0; objectIndex < cloudMessageObject.total; objectIndex++){
            console.log(cloudMessageObject['items'][objectIndex]['message']);
        }
},  cycle_time);