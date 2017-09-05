#include "framework.h"
#include "test_entries.h"

enum TetrisType : uint8_t
{
	_ = 0,
	O,
	T,
	L,
	R,
	I,
	Z,
	N
};

TetrisType OShape[4][4] =
{
	_, _, _, _,
	_, O, O, _,
	_, O, O, _,
	_, _, _, _,
};

TetrisType TShape[4][4] =
{
	_, _, _, _,
	T, T, T, _,
	_, T, _, _,
	_, _, _, _,
};

TetrisType LShape[4][4] =
{
	_, L, _, _,
	_, L, _, _,
	_, L, L, _,
	_, _, _, _,
};

TetrisType RShape[4][4] =
{
	_, R, _, _,
	_, R, _, _,
	R, R, _, _,
	_, _, _, _,
};

TetrisType IShape[4][4] =
{
	_, _, _, _,
	I, I, I, I,
	_, _, _, _,
	_, _, _, _,
};

TetrisType ZShape[4][4] =
{
	_, _, _, _,
	Z, Z, _, _,
	_, Z, Z, _,
	_, _, _, _,
};
TetrisType NShape[4][4] =
{
	_, N, N, _,
	N, N, _, _,
	_, _, _, _,
	_, _, _, _,
};

int RotateState = 0;

TetrisType Active[4][4];
TetrisType ActiveType;
int Cx = 0;
int Cy = 0;

const int SpaceW = 30;
const int SpaceH = 50;
TetrisType Space[SpaceH][SpaceW];
const int BrickWidth = 10;
const int RenderOffsetX = 10;
const int RenderOffsetY = 10;


static void Rotate3x3(TetrisType Shape[4][4], bool CW)
{
	TetrisType Temp[4][4] = { _ };
	if (CW)
	{

		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				Temp[i][j] = Shape[2-j][i];
	}
	else
	{
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				Temp[i][j] = Shape[j][2 - i];
	}

	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			Shape[i][j] = Temp[i][j];
}

static void Rotate4x4(TetrisType Shape[4][4], bool CW)
{
	TetrisType Temp[4][4] = { _ };
	if (CW)
	{

		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				Temp[i][j] = Shape[3 - j][i];
	}
	else
	{
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				Temp[i][j] = Shape[j][3 - i];
	}

	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			Shape[i][j] = Temp[i][j];
}


static void RotateTetris(bool CW)
{
	switch (ActiveType)
	{
	case TetrisType::_:
	case TetrisType::O:
		break;
	case TetrisType::T:
	case TetrisType::L:
	case TetrisType::R:
		Rotate3x3(Active, CW);
		break;
	case TetrisType::I:
		if (RotateState == 0)
		{
			Rotate4x4(Active, true);
			RotateState = 1;
		}			
		else
		{
			Rotate4x4(Active, false);
			RotateState = 0;
		}			
		break;
	case TetrisType::Z:
	case TetrisType::N:
		if (RotateState == 0)
		{
			Rotate3x3(Active, true);
			RotateState = 1;
		}
		else
		{
			Rotate3x3(Active, false);
			RotateState = 0;
		}

		break;
	}
}



static void DrawBoundary(int x1, int y1, int x2, int y2)
{
	float w = BrickWidth;
	ImDrawList *dl = ImGui::GetWindowDrawList();
	

	/*
	+	+
	|	|
	+---+
	*/
	ImVec2 wp =ImGui::GetWindowPos();
	ImVec2 q[4] =
	{
		ImVec2(x1*w, y1*w),
		ImVec2(x1*w, (y2 + 1)*w),
		ImVec2((x2 + 1)*w, (y2 + 1)*w),
		ImVec2((x2 + 1)*w, y1*w),
	};
	for (int i = 0; i < 4; ++i)
	{
		q[i].x += wp.x;
		q[i].y += wp.y;
	};

	ImColor color(0xFFFFFFFF);
	dl->AddQuad(q[0], q[1], q[2],q[3], color);


}

static void DrawBrick(int x, int y, TetrisType TType)
{
	ImDrawList *dl = ImGui::GetWindowDrawList();
	ImVec2 wp = ImGui::GetWindowPos();

	float w = BrickWidth;

	/*
		+	+
		|	|
		+---+
	*/


	ImVec2 q[4] =
	{
		ImVec2(x*w, y*w),
		ImVec2(x*w, (y + 1)*w),
		ImVec2((x + 1)*w, (y + 1)*w),
		ImVec2((x + 1)*w, y*w),
	};
	for (int i = 0; i < 4; ++i)
	{
		q[i].x += wp.x + RenderOffsetX * w;
		q[i].y += wp.y + RenderOffsetY * w;
	};
	
	ImColor red(1.0f, 0.0, 0.0);
	ImColor green(0.0f, 1.0, 0.0);
	ImColor blue(0.0f, 0.0, 1.0);
	ImColor color;
	switch (TType)
	{
	case TetrisType::_:
		return;
		break;
	case TetrisType::O:
		color = red;
		break;
	case TetrisType::T:
		color = green;
		break;
	case TetrisType::L:
		color = blue;
	case TetrisType::R:
		color = blue;
	case TetrisType::N:
		color = blue;
	case TetrisType::Z:
		color = blue;
	case TetrisType::I:
		color = blue;

		break;
	}

	dl->AddQuadFilled(q[0], q[1], q[2], q[3], color);
	float scale = 0.3f;
	color.Value.x *= scale;
	color.Value.y *= scale;
	color.Value.z *= scale;
	dl->AddQuad(q[0], q[1], q[2], q[3], color, 2.0f);
	
}
static void PutTetris(TetrisType Space[SpaceH][SpaceW], TetrisType Tetris[4][4], int x, int y)
{
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			if (Tetris[i][j] != TetrisType::_)
				Space[y + i][x + j] = Tetris[i][j];
		}
	}
}

static void PutTetris(TetrisType Space[4][4], TetrisType Tetris[4][4])
{
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{			
			Space[i][j] = Tetris[i][j];
		}
	}
}




static void PutTetrisType(TetrisType Space[4][4], TetrisType TType)
{
	switch (TType)
	{
	case TetrisType::I:
		PutTetris(Space, IShape);
		break;
	case TetrisType::L:
		PutTetris(Space, LShape);
		break;
	case TetrisType::R:
		PutTetris(Space, RShape);
		break;
	case TetrisType::N:
		PutTetris(Space, NShape);
		break;
	case TetrisType::Z:
		PutTetris(Space, ZShape);
		break;
	case TetrisType::T:
		PutTetris(Space, TShape);
		break;
	case TetrisType::O:
		PutTetris(Space, OShape);
		break;	
		
	}
}

static void FillLine(int y, TetrisType TType)
{
	for (int i = 0; i < SpaceW; ++i)
	{
		Space[y][i] = TType;
	}
}

static int Clamp(int a, int v, int b)
{
	if (v >= b)
		return b-1;
	if (v < a)
		return a;
	return v;
}
static bool InRange(int a, int v, int b)
{
	if (a <= v && v < b)
		return true;
	return false;
}



static bool HasHorizantalCollision()
{
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			TetrisType A = Active[i][j];

			TetrisType S;
			if (!InRange(0, j + Cx, SpaceW) )
				S = TetrisType::O;
			else
				S = Space[Clamp(0, i + Cy, SpaceH)][Clamp(0, j + Cx, SpaceW)];
			if (A != TetrisType::_ && S != TetrisType::_)
				return true;
		}
	}
	return false;
}

static bool HasCollision()
{
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			TetrisType A = Active[i][j];

			TetrisType S;
			if (!InRange(0, j + Cx, SpaceW) || !InRange(0, i + Cy, SpaceH))
				S = TetrisType::O;
			else
				S = Space[Clamp(0, i + Cy, SpaceH)][Clamp(0, j + Cx, SpaceW)];
			if (A != TetrisType::_ && S != TetrisType::_)
				return true;
		}
	}
	return false;
}

static uint64_t Rand()
{
	static uint64_t seed = 123456789;
	uint64_t m = 4294967296;
	uint64_t a = 1103515245;
	uint64_t c = 12345;
	seed = (a * seed + c) % m;
	return seed;
}



static TetrisType GetNextType()
{
	TetrisType NextType = TetrisType::_;
	while ( NextType == TetrisType::_)
		NextType = (TetrisType)(uint8_t)(Rand() % 7);
	return NextType;

}

static void TetrisRoutine(void *InOutParam)
{
	for (int i = 0; i < SpaceH; ++i)
		FillLine(i, TetrisType::_);

	PutTetrisType(Active, TetrisType::O);
	Cx = SpaceW/2;
	Cy = 5;
	RotateState = 0;

	class HandleInput
	{
	public:
		void Do()
		{
			int PreCx = Cx;

			TetrisType Temp[4][4];
			PutTetris(Temp, Active);

			if (frIsKeyPressed(EKeyInput::KB_A))
			{
				RotateTetris(false);
			}

			if (frIsKeyPressed(EKeyInput::KB_D))
			{
				RotateTetris(true);
			}

			if (HasCollision())
			{
				PutTetris(Active, Temp);
			}

			if (frIsKeyPressed(EKeyInput::KB_LEFT))
			{
				Cx--;
			}
			if (frIsKeyPressed(EKeyInput::KB_RIGHT))
			{
				Cx++;
			}

			if (HasHorizantalCollision())
			{
				Cx = PreCx;
			}

			
			if (frIsKeyPressed(EKeyInput::KB_DOWN))
			{
				while (!HasCollision())
					Cy++;
				if (HasCollision())
					Cy--;
			}
		}
	};

	HandleInput input;

	while (1)
	{

		for (int i = 0; i < 5; ++i)
		{
			input.Do();
			WaitForFrame();
		}

		Cy++;

		if (HasCollision())
		{
			// Put it to the stack
			PutTetris(Space, Active, Cx,Cy-1);
			TetrisType NextType = (TetrisType)(uint8_t)(Rand() % 7);

			// Check if stack has lines.

			// Change New 
			ActiveType = GetNextType();
			PutTetrisType(Active, ActiveType);
			Cy = 5;
			Cx = SpaceW / 2;
			RotateState = 0;
		}
			
			
	}
}

static void DoTetris()
{
	DoTask(TetrisRoutine);
}


static void MakeTetrisDrawList()
{
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	
	// Draw Space 
	for (int i = 0; i < SpaceH; ++i)
	{
		for (int j = 0; j < SpaceW; ++j)
		{
			DrawBrick(j, i, Space[i][j]);
		}
	}
	// Draw Active
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			DrawBrick(j + Cx, i + Cy, Active[i][j]);
		}
	}	

	int x = RenderOffsetX;
	int y = RenderOffsetY;
	DrawBoundary(x,y, x+SpaceW-1, y+SpaceH-1);
	
}

void CTetrisTest::Open()
{
	DoTetris();
}

void CTetrisTest::BuildGUI()
{
	MakeTetrisDrawList();
}
