nRF Cloud sample
################

Prerequisites
*************
In order to run the code, you need to have NodeJS and the module node-fetch.

NodeJS: https://nodejs.org/en/

Node Package Manager: https://www.npmjs.com

After you have installed npm and NodeJS, run the command:

.. code-block:: node-fetch

  npm install node-fetch@2
  
After you have installed npm, NodeJS and the module node-fetch, you need to get your API-key from nRF Cloud.

The API-key is found in the account-page in the nRF Cloud workspace: https://nrfcloud.com/#/account

Using the examples
********************************************
Now that you have all the prerequisites, it is time to change API-key on line 42 in the examples to your API-key

.. code-block:: API-key

  const nRFCloudConnection = new NRFCloudAPI("<API-key>");

To test the examples with your API-key, depending on what example you are using run the command:

.. code-block:: cloud-api-1
  
  node cloud-api-message-collecting.js
  
or

.. code-block:: cloud-api-2
  
  node cloud-api-continuous-message-collecting.js

Things to take into account
***************************
If your code cannot fetch any of the messages on nRF Cloud, you could try changing the number 100000 inside the function "getMessages" on line 35.
The number decides what time you want to start collecting messages. 
When it is 100000 you collect messages posted 100 seconds ago to the cloud, try increasing it to see if there are any messages on the cloud.

Remember that the nRF Cloud only stores messages up till 30 days, so trying to collect messages from the cloud that are older than 30 days will not work.