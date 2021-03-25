
#include "../GUI.h"

struct Vec3
{
	float x, y, z;

	float LengthSq() const { return x * x + y * y + z * z; }
	float Length() const { return sqrtf(LengthSq()); }
	Vec3 Normalized() const;
};

inline Vec3 operator + (const Vec3& v) { return { +v.x, +v.y, +v.z }; }
inline Vec3 operator - (const Vec3& v) { return { -v.x, -v.y, -v.z }; }
inline Vec3 operator + (const Vec3& a, const Vec3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline Vec3 operator - (const Vec3& a, const Vec3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline Vec3 operator * (const Vec3& a, const Vec3& b) { return { a.x * b.x, a.y * b.y, a.z * b.z }; }
inline Vec3 operator / (const Vec3& a, const Vec3& b) { return { a.x / b.x, a.y / b.y, a.z / b.z }; }
inline Vec3& operator += (Vec3& a, const Vec3& b) { return a = { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline Vec3& operator -= (Vec3& a, const Vec3& b) { return a = { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline Vec3& operator *= (Vec3& a, const Vec3& b) { return a = { a.x * b.x, a.y * b.y, a.z * b.z }; }
inline Vec3& operator /= (Vec3& a, const Vec3& b) { return a = { a.x / b.x, a.y / b.y, a.z / b.z }; }
inline Vec3 operator * (const Vec3& a, float f) { return { a.x * f, a.y * f, a.z * f }; }
inline Vec3 operator / (const Vec3& a, float f) { return { a.x / f, a.y / f, a.z / f }; }
inline Vec3& operator *= (Vec3& a, float f) { return a = { a.x * f, a.y * f, a.z * f }; }
inline Vec3& operator /= (Vec3& a, float f) { return a = { a.x / f, a.y / f, a.z / f }; }
inline Vec3 Vec3::Normalized() const
{
	float lsq = LengthSq();
	return lsq ? *this / sqrtf(lsq) : Vec3{ 0, 0, 0 };
}
inline float Vec3Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 Vec3Cross(const Vec3& a, const Vec3& b)
{
	return
	{
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x,
	};
}

inline float randf()
{
	return rand() / float(RAND_MAX);
}
inline Vec3 Vec3RandomSphere()
{
	float a = randf() * 3.14159f * 2;
	float b = randf() * 3.14159f;
	float sb = sinf(b);
	return { sb * cosf(a), sb * sinf(a), cosf(b) };
}
inline Vec3 Vec3RandomHemi(const Vec3& dir)
{
	Vec3 rs = Vec3RandomSphere();
	return Vec3Dot(rs, dir) >= 0 ? rs : -rs;
}
inline Vec3 Vec3Reflect(const Vec3& dir, const Vec3& n)
{
	return -dir + n * 2 * Vec3Dot(dir, n);
}

float RayPlaneIntersect(const Vec3& raypos, const Vec3& raydir, const Vec3& pln, float pld)
{
	float d = Vec3Dot(pln, raydir);
	if (fabsf(d) > 0.0001f)
	{
		float t = (pld - Vec3Dot(raypos, pln)) / d;
		return t;
	}
	return -FLT_MAX;
}

struct Material
{
	Vec3 diffuse;
	Vec3 emissive;
	bool mirror;
};

struct RaycastResult
{
	float dist;
	Vec3 point;
	Vec3 normal;
	Material material;
};

struct Level
{
	static constexpr int GRID_W = 16;
	static constexpr int GRID_H = 16;

	Level()
	{
		memcpy(grid,
			"################"
			"#              #"
			"#  #  ###      #"
			"#              #"
			"# L L L L L L  #"
			"# r# # # # # # #"
			"# L L L L L L  #"
			"#  # # # # # # #"
			"# L L L L L L  #"
			"# r# # # # # # #"
			"# L L L L L L  #"
			"#  # # # # # # #"
			"# L L L L L L  #"
			"#  # # # # # # #"
			"# mmmmmmmmmmmm #"
			"################"
			, GRID_W * GRID_H);
	}

	RaycastResult Raycast(Vec3 from, Vec3 dir)
	{
		RaycastResult rr = { FLT_MAX, from, -dir, skyMtl };

		float floorDist = RayPlaneIntersect(from, dir, { 0, 0, 1 }, -1);
		if (floorDist > 0 && floorDist < rr.dist)
		{
			rr.dist = floorDist;
			rr.normal = { 0, 0, 1 };
			rr.material = floorMtl;
		}

		float ceilDist = RayPlaneIntersect(from, dir, { 0, 0, -1 }, -1);
		if (ceilDist > 0 && ceilDist < rr.dist)
		{
			rr.dist = ceilDist;
			rr.normal = { 0, 0, -1 };
			auto pos = from + dir * ceilDist;
			int px = floor(pos.x);
			int py = floor(pos.y);
			rr.material = px >= 0 && py >= 0 && px < GRID_W && py < GRID_H && grid[px + GRID_W * py] == 'L' ? lightMtl : ceilMtl;
		}

		auto wrr = RaycastWalls(from, dir);
		if (wrr.dist < rr.dist)
		{
			rr.dist = wrr.dist;
			rr.normal = wrr.normal;
			rr.material = wrr.material;
		}

		rr.point = from + dir * rr.dist;
		return rr;
	}

	struct WallRaycastResult
	{
		float dist;
		Vec3 normal;
		Material material;
	};
	WallRaycastResult RaycastWalls(Vec3 from, Vec3 dir)
	{
		if (fabsf(dir.x) < 0.0001f && fabsf(dir.y) < 0.0001f)
			return { FLT_MAX };
		int stepX = dir.x == 0 ? 0 : dir.x > 0 ? 1 : -1;
		int stepY = dir.y == 0 ? 0 : dir.y > 0 ? 1 : -1;
		float tDeltaX = dir.x ? 1 / fabsf(dir.x) : 0;
		float tDeltaY = dir.y ? 1 / fabsf(dir.y) : 0;
		float tMaxX = FLT_MAX, tMaxY = FLT_MAX;
		int x = 0, y = 0;
		if (dir.x != 0)
		{
			float b;
			if (dir.x > 0)
			{
				b = ceil(from.x);
				if (b < 0)
					b = 0;
				x = floor(b - 1);
			}
			else
			{
				b = floor(from.x);
				if (b > GRID_W)
					b = GRID_W;
				x = ceil(b);
			}
			tMaxX = (b - from.x) / dir.x;
		}
		if (dir.y != 0)
		{
			float b;
			if (dir.y > 0)
			{
				b = ceil(from.y);
				if (b < 0)
					b = 0;
				y = floor(b - 1);
			}
			else
			{
				b = floor(from.y);
				if (b > GRID_H)
					b = GRID_H;
				y = ceil(b);
			}
			tMaxY = (b - from.y) / dir.y;
		}

		while (x >= -1 && x <= GRID_W && y >= -1 && y <= GRID_H)
		{
			int px = x, py = y;
			float pmin = tMaxX < tMaxY ? tMaxX : tMaxY;
			if (tMaxX < tMaxY)
			{
				tMaxX += tDeltaX;
				x += stepX;
			}
			else
			{
				tMaxY += tDeltaY;
				y += stepY;
			}

			if (x >= 0 && y >= 0 && x < GRID_W && y < GRID_H && grid[x + y * GRID_W] == '#')
			{
				char m = ' ';
				if (px >= 0 && py >= 0 && px < GRID_W && py < GRID_H)
					m = grid[px + py * GRID_W];
				Material mtl = wallMtl;
				if (m == 'r')
					mtl = redWallMtl;
				if (m == 'm')
					mtl = mirrWallMtl;
				return { pmin, Vec3{ float(px - x), float(py - y), 0.0f }, mtl };
			}
		}
		return { FLT_MAX };
	}

	char grid[16 * 16];
	Material skyMtl = { Vec3{0,0,0}, Vec3{ 0.5f, 0.7f, 0.9f } };
	Material floorMtl = { Vec3{ 0.6f, 0.6f, 0.6f }, Vec3{0,0,0} };
	Material ceilMtl = { Vec3{ 0.8f, 0.8f, 0.8f }, Vec3{0,0,0} };
	Material lightMtl = { Vec3{ 0.1f, 0.1f, 0.1f }, Vec3{ 500000.f, 50000.f, 5000.f } };
	Material wallMtl = { Vec3{ 0.7f, 0.7f, 0.7f }, Vec3{0,0,0} };
	Material redWallMtl = { Vec3{ 0.9f, 0.1f, 0.0f }, Vec3{0,0,0} };
	Material mirrWallMtl = { Vec3{ 1, 1, 1 }, Vec3{0,0,0}, true };
}
g_level;

Vec3 cameraPos = { 1.2f, 3.5f, 0.0f };
Vec3 cameraDir = Vec3{ 2, 1, 0 };
Vec3 cameraUp = { 0, 0, 1 };
float cameraFOV = 90.0f;
ui::DataCategoryTag DCT_CameraEdited[1];

Vec3 GetCameraRayDir(int x, int y, int w, int h)
{
	if (w < 1) w = 1;
	if (h < 1) h = 1;
	float asp = float(w) / float(h);
	float fx = float(x) / float(w) * 2 - 1;
	float fy = float(y) / float(h) * 2 - 1;
	float d = tanf(cameraFOV * 3.14159f / 180 * 0.5f);
	Vec3 camDirNorm = cameraDir.Normalized();
	Vec3 cameraRight = Vec3Cross(cameraUp, camDirNorm);
	return (camDirNorm + cameraRight * fx * d * asp - cameraUp * fy * d).Normalized();
}

struct RenderView : ui::Buildable
{
	~RenderView()
	{
		delete image;
	}
	void Build(ui::UIContainer* ctx) override
	{
		Subscribe(DCT_CameraEdited);

		imageEl = &ctx->Make<ui::ImageElement>();
		imageEl->GetStyle().SetWidth(style::Coord::Percent(100));
		imageEl->GetStyle().SetHeight(style::Coord::Percent(100));
		imageEl->SetScaleMode(ui::ScaleMode::Stretch);
		imageEl->SetImage(image);
	}
	void OnNotify(ui::DataCategoryTag* tag, uintptr_t at) override
	{
		ui::Buildable::OnNotify(tag, at);
		if (tag == DCT_CameraEdited)
			UpdateImage(true);
	}
	void OnLayoutChanged() override
	{
		UpdateImage();
	}
	void UpdateImage(bool force = false)
	{
		ui::Application::PushEvent(this, [this, force]()
		{
			auto cr = imageEl->GetContentRect();
			int scale = 1;
#if !NDEBUG
			scale = 2;
#endif
			int tw = int(cr.GetWidth() / scale);
			int th = int(cr.GetHeight() / scale);

			if (!force && image && image->GetWidth() == tw && image->GetHeight() == th)
				return;

			wq.Push([this, tw, th]()
			{
				ui::Canvas canvas(tw, th);

				auto* px = canvas.GetPixels();
				memset(px, 0, tw * th * 4);

				std::vector<Vec3> tmpimg;
				tmpimg.resize(tw * th, { 0, 0, 0 });
				std::vector<float> tmpscale;
				tmpscale.resize(tw * th, 0.0001f);

				for (int nwhole = 0; nwhole < 64; nwhole++)
				{
					for (uint32_t y = 0; y < th; y++)
					{
						if (wq.HasItems() || wq.IsQuitting())
							return;

						for (uint32_t x = 0; x < tw; x++)
						{
							Vec3 raypos = cameraPos;
							Vec3 raydir = GetCameraRayDir(x, y, tw, th);
							Vec3 d = { 1, 1, 1 };
							Vec3 c = { 0, 0, 0 };
							for (int i = 0; i < 4; i++)
							{
								auto rr = g_level.Raycast(raypos, raydir);
								float dot = -Vec3Dot(raydir, rr.normal);
								if (dot < 0)
									break;
								c += d * rr.material.emissive * dot;
								d *= rr.material.diffuse * dot;
								raypos = rr.point + rr.normal * 0.0001f;
								raydir = rr.material.mirror ? Vec3Reflect(-raydir, rr.normal) : Vec3RandomHemi(rr.normal);
								d *= Vec3Dot(raydir, rr.normal);
							}
							float exp = 0.1f;
							c.x = log(c.x * exp + 1) / log(100);
							c.y = log(c.y * exp + 1) / log(100);
							c.z = log(c.z * exp + 1) / log(100);
							if (c.x > 1) c.x = 1;
							if (c.y > 1) c.y = 1;
							if (c.z > 1) c.z = 1;
							Vec3 fc = tmpimg[x + y * tw] += c;
							float fs = tmpscale[x + y * tw] += 1;
							fc /= fs;
							px[x + y * tw] = (0xff << 24) | (int(fc.z * 255) << 16) | (int(fc.y * 255) << 8) | int(fc.x * 255);
						}
					}

					ui::Application::PushEvent(this, [this, canvas]()
					{
						delete image;
						image = new ui::Image(canvas);
						Rebuild();
					});
				}
			}, true);
		});
	}

	ui::WorkerQueue wq;
	ui::Image* image = nullptr;
	ui::ImageElement* imageEl = nullptr;
};

struct EditorView : ui::Buildable
{
	void Build(ui::UIContainer* ctx) override
	{
	}
};

struct InspectorView : ui::Buildable
{
	void Build(ui::UIContainer* ctx) override
	{
		ctx->Push<ui::PropertyList>();

		ctx->Text("Camera");
		auto& cameraBox = ctx->PushBox();
		ui::imm::PropEditFloat(ctx, "FOV", cameraFOV);
		ui::imm::PropEditFloatVec(ctx, "Position", &cameraPos.x);
		ui::imm::PropEditFloatVec(ctx, "Direction", &cameraDir.x);
		cameraBox.HandleEvent(ui::EventType::Commit) = [](ui::Event& e)
		{
			ui::Notify(DCT_CameraEdited);
		};
		ctx->Pop();

		ctx->Pop();
	}
};

struct MainWindow : ui::NativeMainWindow
{
	void OnBuild(ui::UIContainer* ctx) override
	{
		auto& hsp = ctx->Push<ui::SplitPane>();
		{
			auto& vsp = ctx->Push<ui::SplitPane>();
			vsp.SetDirection(true);
			{
				ctx->Make<RenderView>();
				ctx->Make<EditorView>();

				vsp.SetSplits({ 0.6f });
			}
			ctx->Pop();

			ctx->Make<InspectorView>();

			hsp.SetSplits({ 0.6f });
		}
		ctx->Pop();
	}
};

int uimain(int argc, char* argv[])
{
	ui::Application app(argc, argv);
	MainWindow mw;
	mw.SetSize(1024, 576);
	mw.SetVisible(true);
	return app.Run();
}
