// assignment3.cpp: A program using the TL-Engine

#include <TL-Engine.h>	// TL-Engine include file and namespace
#include <math.h>
#include <sstream>
using namespace tle;

void main()
{
	// Create a 3D engine (using TLX engine here) and open a window for it
	I3DEngine* myEngine = New3DEngine( kTLX );
	myEngine->StartWindowed();
	myEngine->StopMouseCapture();
	myEngine->StartMouseCapture();

	// Add default folder for meshes and other media
	myEngine->AddMediaFolder( "C:\\ProgramData\\TL-Engine\\Media" );

	//Loads all the needed meshes for the program
	IMesh* checkpointMesh = myEngine->LoadMesh("Checkpoint.x");
	IMesh* isleMesh = myEngine->LoadMesh("IsleStraight.x");
	IMesh* wallMesh = myEngine->LoadMesh("Wall.x");
	IMesh* floorMesh = myEngine->LoadMesh("floor.x");
	IMesh* skyboxMesh = myEngine->LoadMesh("Skybox.x");
	IMesh* hovercarMesh = myEngine->LoadMesh("race2.x");
	IMesh* dummyMesh = myEngine->LoadMesh("dummy.x");

	//Creates all the needed models for the program
	//Assigns the meshes to models and positions them onscreen

	IModel* floor = floorMesh->CreateModel(0, 0, 0);
	IModel* skybox = skyboxMesh->CreateModel(0, -960, 0);
	IModel* hovercar = hovercarMesh->CreateModel(0, 0, -50);
	IModel* dummy = dummyMesh->CreateModel(0, 0, -50);

	//Creates a struct to hold the IModels for one wall instead of creating countless IModels for each individual wall
	struct Barrier
	{
		IModel* isleOne;
		IModel* isleTwo;
		IModel* wall;
	};

	const int barriers = 2;
	const int checkpoints = 2;
	int Check = 0;

	Barrier barrier[barriers];

	//Holds all the positions for any walls I need
	const float isleOneXaxis[barriers]{ -10.0f, 10.0f, };
	const float isleOneZaxis[barriers]{ 40.0f, 40.0f };

	const float wallXaxis[barriers]{ -10.0f, 10.0f };
	const float wallZaxis[barriers]{ 46.0f, 46.0f };

	const float isleTwoXaxis[barriers]{ -10.0f, 10.0f };
	const float isleTwoZaxis[barriers]{ 53.0f, 53.0f };

	//With the struct I can make a for loop that will create as many walls as I need
	for (int b = 0; b < barriers; b++)
	{
		barrier[b].isleOne = isleMesh->CreateModel(isleOneXaxis[b], 0.0f, isleOneZaxis[b]);
		barrier[b].wall = wallMesh->CreateModel(wallXaxis[b], 0.0f, wallZaxis[b]);
		barrier[b].isleTwo = isleMesh->CreateModel(isleTwoXaxis[b], 0.0f, isleTwoZaxis[b]);
	}

	//The same principle is used here for the checkpoint, I can have as many as I want with only one IModel being used
	IModel* checkpointmodel[checkpoints];

	const float checkpointXaxis[checkpoints]{ 0.0f, 0.0f };
	const float checkpointYaxis[checkpoints]{ 0.0f, 100.0f };

	for (int c = 0; c < checkpoints; c++)
	{
		checkpointmodel[c] = checkpointMesh->CreateModel(checkpointXaxis[c], 0.0f, checkpointYaxis[c]);
	}

	;	//Loads in 2D sprites, in this case I used a .png file
		//to act as a UI backdrop which will display information
		//such as speed, current lap, stage, etc.
	ISprite* uibackdrop = myEngine->CreateSprite("CustomUI.png", 140, 570);

	//Loads the specified font and font size for later use
	IFont* FontTNM20 = myEngine->LoadFont("Times New Roman", 20);
	IFont* FontTNM25 = myEngine->LoadFont("Times New Roman", 25);
	IFont* FontTNM100 = myEngine->LoadFont("Times New Roman", 100);

	//Positions and rotates the camera behind the hover car
	ICamera* camera = myEngine->CreateCamera(kManual, 0, 15, 30);
	camera->RotateX(25);
	camera->AttachToParent(dummy);
	dummy->AttachToParent(hovercar);

	float cameraspeed = 0.1f;
	int cameraX = 0;
	int cameraYthird = 15;
	int cameraYfirst = 5;
	int cameraZthird = 30;
	int cameraZfirst = 55;

	//Changes the default wood text to the sand
	floor->SetSkin("Sand.png");

	//Keys assigned to a name, making it easier to read
	const EKeyCode EscapeKey(Key_Escape);
	const EKeyCode Pause(Key_P);
	const EKeyCode Forward(Key_W);
	const EKeyCode Back(Key_S);
	const EKeyCode Left(Key_A);
	const EKeyCode Right(Key_D);
	const EKeyCode Space(Key_Space);
	const EKeyCode MouseToggle(Key_Tab);
	const EKeyCode Handbrake(Key_Shift);
	//CP = Camera Position
	const EKeyCode CP3rdperson(Key_1);
	const EKeyCode CP1stperson(Key_2);
	const EKeyCode CPupmovement(Key_Up);
	const EKeyCode CPdownmovement(Key_Down);
	const EKeyCode CPleftmovement(Key_Left);
	const EKeyCode CPrightmovement(Key_Right);

	const float topspeed = 150.1f;
	const float boosttopspeed = 200.1f;
	const float reversetopspeed = -35.1f;
	float steering = 0.0f;
	float steerlimit = 90.1f;
	float accelaration = 0.0f;
	float epsilon = 0.1;

	//Collision speeds
	const float minusSpeed = -20.0f;
	const float positiveSpeed = 20.0f;
	const float collisionspeed = 0.0f;
	const float localZmovement = 2;

	const float handbrakespeed = 0.075f;
	const float accelspeed = 0.006f;
	const float stoppedspeed = 0.0f;
	const float slowdown = 0.004f;
	const float steeringspeed = 0.07f;

	//Creates the timer variable which will be used to calculate the FPS of the time
	float frameTime = myEngine->Timer();
	float countdown = 4000.0f;
	float countdownthree = 3000.0f;
	float countdowntwo = 2000.0f;
	float countdownone = 1000.0f;
	float startgame = 0.0f;

	int durability = 100;

	//Easier to read what state the game is in further in the code
	int notextpausedstate = 0;
	int textpausedstate = 1;

	//1 frame which is then timesd with frametime for the gamespeed
	int FPSvalue = 1;

	//All the co-ordinates of where the text is placed, this string
	//was made purely to remove any duplicated numbers and saves 5 lines of extra code
	int textcoords[12]{ 215, 300, 400, 460, 610, 620, 625, 650, 665, 690, 800, 1065 };

	//SF = smallfont
	//MF = mediumfont
	//LF = largefont
	int LFYaxis = 300;
	int LFXaxis = 650;

	int MFXaxis = 215;
	int MFYaxis = 610;
	int MFYaxisnum = 625;
	int MFXaxisdur = 1065;

	int SFXaxisState = 400;
	int SFXaxisstatenum = 460;
	int SFXaxisFPS = 620;
	int SFXaxisFPSnum = 665;
	int SFYaxis = 695;
	int SFXaxisPos = 800;

	//Colour of the text
	int bluetextcolour = kBlue;
	int redtextcolour = kRed;
	int whitetextcolour = kWhite;

	bool disablemousecursor = true;
	bool paused = false;
	bool cp3rdperson = true;
	bool clearedcheckpoint = false;
	bool passedCheckpoint = false;

	string pausetextstates[2]{ "NoText", "Text" };
	string pausetextstate;

	enum gamestate { Start, CountDown, Playing, Paused, GameOver };
	gamestate gamestate = Start;
	string playingtext = "Playing";
	string pausedtext = "Paused";
	string currentgamestate;

	enum currentcheckpoint { NoCheckpoint, Checkpoint1, Checkpoint2, EndRace };
	currentcheckpoint checkpoint = NoCheckpoint;
	int checkpointincrease = 0;

	//cp = checkpoint
	int cp = 0;
	int cpzero = 0;
	int cpone = 1;
	//bc = barrier collision
	int bczero = 0;
	float time = 0.0f;
	float halfsecond = 0.5f;

	float barrierX = 5.0f;
	float barrierY = 5.0f;

	float checkpointX = 7.5f;
	float checkpointZ = 1.0f;

	int mousemultiplier = 20;

	//Initialises the states in which each string should start
	pausetextstate = pausetextstates[notextpausedstate];

	//////////////////////////////////////////////////////////
	///// GAME LOOP - GAME LOOP - GAME LOOP - GAME LOOP /////
	////////////////////////////////////////////////////////

	while (myEngine->IsRunning())
	{
		// Draw the scene
		myEngine->DrawScene();

		//Tracks mouse movement along the Y axis
		int mousemovement = myEngine->GetMouseMovementX();
		const int mouseRotationMultiplier = mousemultiplier;

		//Converts the accelaration float value to integer, removing the decimal place
		int accel = static_cast<int>(accelaration);

		//Keeps the text on screen constantly so it doesn't disappear
		FontTNM25->Draw("Speed", MFXaxis, MFYaxis, whitetextcolour, kCentre);
		FontTNM25->Draw("Durability", MFXaxisdur, MFYaxis, whitetextcolour, kCentre);

		FontTNM20->Draw("FPS:", SFXaxisFPS, SFYaxis, whitetextcolour, kCentre);
		FontTNM20->Draw("State:", SFXaxisState, SFYaxis, whitetextcolour, kCentre);
		FontTNM20->Draw("Position:", SFXaxisPos, SFYaxis, whitetextcolour, kCentre);

		if (gamestate == Playing)
		{
			if (pausetextstate == pausetextstates[textpausedstate])
			{
				FontTNM100->Draw("Paused, press P to carry on playing", LFXaxis, LFYaxis, redtextcolour, kCentre);
			}

			if (myEngine->KeyHit(Pause))
			{
				paused = !paused;

				pausetextstate = pausetextstates[textpausedstate];
				gamestate = Paused;

				if (gamestate == Paused)
				{
					gamestate = Playing;
				}
			}
		}

		if (!paused)
		{
			//When the game first starts, it's not running and tells you to hit Space to start the game
			//It will then change the state to the playing state
			if (gamestate == Start)
			{
				FontTNM100->Draw("Hit Space to Start", LFXaxis, LFYaxis, bluetextcolour, kCentre);
				if (myEngine->KeyHit(Space))
				{
					gamestate = CountDown;
				}
			}
			//Once space is pressed, the timer is initiated which counts down from 3 to 1 and then displays GO
			else if (gamestate == CountDown)
			{
				countdown -= frameTime;

				if (countdown > countdownthree)
				{
					FontTNM100->Draw("3", LFXaxis, LFYaxis, whitetextcolour, kCentre);
				}
				else if (countdown > countdowntwo)
				{
					FontTNM100->Draw("2", LFXaxis, LFYaxis, whitetextcolour, kCentre);
				}
				else if (countdown > countdownone)
				{
					FontTNM100->Draw("1", LFXaxis, LFYaxis, whitetextcolour, kCentre);
				}
				else { FontTNM100->Draw("GO", LFXaxis, LFYaxis, whitetextcolour, kCentre); }
			}
			//Once the countdown is less than the startgame value, it changes states to playing
			if (countdown < startgame)
			{
				gamestate = Playing;
			}

			if (gamestate == Playing)
			{
				mousemovement;
				dummy->RotateY((mousemovement * frameTime)* mouseRotationMultiplier);

				//The acceleration and steering are timesd by the frameTime so it can move at a more realistic speed
				//It will also move at the same speed on various different computer configuations
				//This is instead of having to manually change the gamespeed on different computers
				hovercar->MoveLocalZ(accelaration * frameTime);
				hovercar->RotateY(steering * frameTime);

				pausetextstate = pausetextstates[notextpausedstate];

				//The frametime, which is the timer is divided by the FPSvalue of 1 to calculate the FPS
				int FPS = FPSvalue / frameTime;
				//Updates the timer every time the while loop loops around
				frameTime = myEngine->Timer();

				//Displays the current state of the game
				if (gamestate == Playing)
				{
					FontTNM20->Draw(playingtext, SFXaxisstatenum, SFYaxis, whitetextcolour, kCentre);
				}
				if (gamestate == Paused)
				{
					FontTNM20->Draw(pausedtext, SFXaxisstatenum, SFYaxis, whitetextcolour, kCentre);
				}

				//Displays the current speed of the hovercar on screen which is constantly updated as the hovercar moves
				stringstream SpeedText;
				SpeedText << accel;
				FontTNM100->Draw(SpeedText.str(), MFXaxis, MFYaxisnum, whitetextcolour, kCentre);

				//Displays the current durability of the hovercar on screen which is constantly updated as the hovercar is damaged
				stringstream DurabilityText;
				DurabilityText << durability;
				FontTNM100->Draw(DurabilityText.str(), MFXaxisdur, MFYaxisnum, whitetextcolour, kCentre);

				//Displays the FPS number on screen
				stringstream FPSText;
				FPSText << FPS;
				FontTNM20->Draw(FPSText.str(), SFXaxisFPSnum, SFYaxis, whitetextcolour, kCentre);

				if (gamestate == Playing)
				{
					//Changes the camera position to a 3rd or 1st person view
					if (myEngine->KeyHit(CP3rdperson))
					{
						camera->SetLocalPosition(cameraX, cameraYthird, cameraZthird);
						cp3rdperson = true;
					}
					if (myEngine->KeyHit(CP1stperson))
					{
						camera->SetLocalPosition(cameraX, cameraYfirst, cameraZfirst);
						cp3rdperson = false;
					}

					//Allows the camera to be moved around on the X and Z axis
					if (cp3rdperson)
					{
						if (myEngine->KeyHeld(CPupmovement))
						{
							camera->MoveZ(cameraspeed);
						}
						if (myEngine->KeyHeld(CPdownmovement))
						{
							camera->MoveZ(-cameraspeed);
						}
						if (myEngine->KeyHeld(CPleftmovement))
						{
							camera->MoveX(cameraspeed);
						}
						if (myEngine->KeyHeld(CPrightmovement))
						{
							camera->MoveX(-cameraspeed);
						}
					}

					//if the forward key is pressed, the hovercar will move forward along it's local Z axis
					if (myEngine->KeyHeld(Forward))
					{
						accelaration += accelspeed;
						if (accelaration >= topspeed)
						{
							accelaration = topspeed;
						}
					}
					//if the backwards key is pressed, the hovercar will move backwards along it's local Z axis
					else if (myEngine->KeyHeld(Back))
					{
						accelaration -= accelspeed;
						if (accelaration <= reversetopspeed)
						{
							accelaration = reversetopspeed;
						}
					}
					//If none of the keys are pressed and the hovercar is moving, depending on the direction
					//one of the else if statement will slow the hovercar down and then the if statement will completely stop it
					else
					{
						//if the hovercar is inbetween the two low values the speed will automatically set to 0
						if (accelaration < -epsilon && accelaration > epsilon)
						{
							accelaration = stoppedspeed;
						}
						//if the hovercar is above that speed and the forward key isn't held down, the hovercar will slow down to 0
						else if (accelaration > epsilon)
						{
							accelaration -= slowdown;
						}
						//if the hovercar is below that speed and the backwards key isn't held down, the hovercar will slown down to 0
						else if (accelaration < -epsilon)
						{
							accelaration += slowdown;
						}
					}
					//Stops you from turning whilst you aren't moving. You can only steer when you're moving
					if (accelaration != stoppedspeed)
					{

						if (myEngine->KeyHeld(Left) && steering >= -steerlimit)
						{
							steering -= steeringspeed;
						}
						else if (myEngine->KeyHeld(Right) && steering <= steerlimit)
						{
							steering += steeringspeed;
						}
						else
						{
							if (steering < epsilon && steering > -epsilon)
							{
								steering = stoppedspeed;
							}
							else if (steering > epsilon)
							{
								steering -= epsilon;
							}
							else if (steering < -epsilon)
							{
								steering += epsilon;
							}
						}
					}
					//If you stop moving and you're still turning. You'll also stop turning
					else if (accelaration == epsilon)
					{
						steering = stoppedspeed;
					}

					//Acts as a handbrake in a car to slow the hovercar down faster than the reverse/brake key
					if (myEngine->KeyHeld(Handbrake))
					{
						if (accelaration > epsilon)
						{
							accelaration -= handbrakespeed;
						}
						else if (accelaration < epsilon)
						{
							accelaration += handbrakespeed;
						}
					}

					//Barrier Collision Detection
					for (int bc = bczero; bc < barriers; bc++)
					{
						if (hovercar->GetX() >(barrier[bc].isleOne->GetX() - barrierX) && hovercar->GetX() < (barrier[bc].isleTwo->GetX() + barrierX) &&
							hovercar->GetZ() > (barrier[bc].isleOne->GetZ() - barrierY) && hovercar->GetZ() < barrier[bc].isleTwo->GetZ() + barrierY)
						{
							if (accelaration > collisionspeed)
							{
								accelaration = minusSpeed;
								hovercar->MoveLocalZ(-localZmovement); //Stops the hovercar from getting stuck in the wall
							}
							else if (accelaration < collisionspeed)
							{
								accelaration = positiveSpeed;
								hovercar->MoveLocalZ(localZmovement);
							}
							else
							{
								accelaration = minusSpeed;
							}

							durability--;
						}
					}
					//Detects if the hovercar has passed through the checkpoint or not
					if (hovercar->GetX() >(checkpointmodel[cp]->GetX() - checkpointX) && hovercar->GetX() < (checkpointmodel[cp]->GetX() + checkpointX) &&
						hovercar->GetZ() > (checkpointmodel[cp]->GetZ() - checkpointZ) && hovercar->GetZ() < (checkpointmodel[cp]->GetZ() + checkpointZ))
					{
						//If the hovercar has passed through the checkpoint, the boolean changes to true
						clearedcheckpoint = true;
					}

					//If the boolean is true it changes the checkpoint state and display text on the screen
					if (clearedcheckpoint)
					{
						//If the hovercar passes the first checkpoint, the states will change and text will be displayed on screen for half a second
						if (cp == cpzero)
						{
							if (checkpoint == NoCheckpoint)
							{
								checkpoint = Checkpoint1;
							}
							if (checkpoint == Checkpoint1)
							{
								time += frameTime;
								FontTNM100->Draw("Stage 1 Complete", LFXaxis, LFYaxis, whitetextcolour, kCentre);
								if (time >= halfsecond)
								{
									cp++;
									clearedcheckpoint = false;
								}
							}
						}
						//If the hovercar passes the last checkpoint, the states will change and text will be displayed on screen 
						if (cp == cpone)
						{
							checkpoint = Checkpoint2;
							if (checkpoint == Checkpoint2)
							{
								checkpoint = EndRace;
							}
							if (checkpoint == EndRace)
							{
								gamestate = GameOver;
								FontTNM100->Draw("RACE COMPLETE!", LFXaxis, LFYaxis, whitetextcolour, kCentre);
							}
						}
					}
				}
			}

			//Hides the mouse cursor until the mousetoggle key is pressed, which then displays the mouse cursor on screen
			if (disablemousecursor)
			{
				if (myEngine->KeyHit(MouseToggle))
				{
					myEngine->StopMouseCapture();
					disablemousecursor = false;
				}
			}
			//Displays the mouse cursor until the mousetoggle key is pressed, which then hides the mouse cursor on screen
			else
			{
				if (myEngine->KeyHit(MouseToggle))
				{
					myEngine->StartMouseCapture();
					disablemousecursor = true;

				}
			}
		}

		//if the escape key is pressed, it will close the application window
		if (myEngine->KeyHit(EscapeKey))
		{
			myEngine->Stop();
		}

	}

	// Delete the 3D engine now we are finished with it
	myEngine->Delete();
}
