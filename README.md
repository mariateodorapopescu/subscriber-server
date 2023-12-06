# Communication Magic with SERVER SUBSFRIBER NOTIFICATION APP IN C++ 📡✨

Welcome to the enchanting world of Communication Protocols! This spellbinding repository is dedicated to the mystical implementation of Server and Subscriber for Theme 2.

## Server Chronicles 🧙‍♂️🏰

### Implementation Sorcery

- The realm of information about messages, clients, and topics is guarded by mighty `unordered_map`s and `unordered_set`s, granting swift access in O(1) time.
  
- Multiplexing I/O spells are cast to gracefully manage the influx of inputs from UDP publishers, TCP subscribers, and the all-seeing stdin.

- Sockets, both UDP and TCP, are conjured into existence, with the TCP socket remaining passive, ready to listen for new connection requests from subscribers.

- The server weaves its magic until the command `exit` echoes through the mystical land.

- Messages from UDP publishers are transmuted into the required format for TCP subscribers, aligning with the prophecy.

- For topics requiring message storage (where a TCP client with SF was offline), the server consults the corresponding map, ensuring no message is left behind.

- Messages are then sent to all online subscribers who are subscribed to the topic of the received message.

- In the case of a TCP connection request, the server graciously accepts, checking if the new client was once online. If so, it updates their subscribed topics and delivers lost messages.

- Should a subscriber TCP wish to unsubscribe, the server consults the map to ensure no unnecessary entries persist.

- Upon a subscriber's departure, their topics, SF settings, and the message number for retransmission are stored, ready for their triumphant return.

### Error Handling Enchantments

- Critical operations, such as socket creation, select calls, bind, or listen, are guarded by magical wards. Failure in any of these incantations leads to the graceful closure of the server, severing all active connections.

- Mischievous UDP messages with incorrect formats are ignored, and a discreet error message is whispered to `stderr`.

- Messages from UDP clients are assumed pure and free of the disruptive `\0` in both data and topic fields.

## Subscriber Chronicles 🧙‍♀️📖

### Implementation Spells

- The humble subscriber, armed with a communication socket, severs ties with the Nagle algorithm and deftly juggles input from both the server and the mortal realm of keyboards.

- Messages from the server arrive in a format ready for display, and the subscriber promptly unveils them to the mortal realm.

### Error Handling Charms

- In case of critical failures during operations, the subscriber bows out gracefully, leaving a subtle error message in its wake.

- Each command undergoes scrutiny, and in the face of errors (incorrect parameter count, unauthorized commands, or topics stretching beyond 50 characters), a gentle rebuke is whispered to `stderr`, and the command is silently dismissed.

May your communication be swift, and your protocols enchanting! 🌐✨
