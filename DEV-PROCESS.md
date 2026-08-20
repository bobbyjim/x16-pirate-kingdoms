Okay here's how I'm thinking we should develop this.

First, we need to establish the core simulation mechanics that will drive the emergent behavior of the settlements. This includes defining the attributes of each settlement, how they interact with each other, and how events will impact them over time.

Yes but also we need to sharply delineate the central business logic from the UI logic. The business logic should be agnostic of any specific user interface, allowing for flexibility in how the game is presented to players. This is because the target hardware
is the Commander X16, and the compiler is cc65. Thus we can only create unit tests on 
the vanilla business logic, and the UI will be a separate layer that interacts with this core logic. What's more, I suspect we'll have a CLI interface for testing and debugging, which will also need to be separate from the core business logic, and separate from the UI as well.

