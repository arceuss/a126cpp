// GL inventory checker.
//
// The LegacyGL frontend's surface is defined by what the game actually issues,
// so that definition has to be enforced mechanically. This tool scans the source
// tree for GL-shaped entry points and enums and compares them against the
// declarations in legacygl/LegacyGL.h.
//
// A renderer path that reaches for an unhandled GL call fails here, with the
// file and line, instead of silently no-opping inside a translated backend. It
// also prints the usage table docs/portable/gl-api-coverage.md is built from.
//
// Usage: a126cpp-gl-inventory [source-root] [--table]

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifndef A126_SOURCE_ROOT
#define A126_SOURCE_ROOT "src"
#endif

struct SymbolUsage
{
	long long count = 0;
	std::string firstFile;
	int firstLine = 0;
	std::set<std::string> files;
};

static bool isIdentifierChar(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

static std::string readFile(const std::filesystem::path &path)
{
	std::ifstream stream(path, std::ios::in | std::ios::binary);
	if (!stream.is_open())
		return std::string();

	std::ostringstream contents;
	contents << stream.rdbuf();
	return contents.str();
}

// Replaces comments and string/character literals with spaces, keeping newlines
// so line numbers stay meaningful. Without this, the Java reference comments
// throughout the renderer would be reported as GL usage.
static std::string stripCommentsAndLiterals(const std::string &text)
{
	std::string out;
	out.reserve(text.size());

	std::size_t i = 0;
	const std::size_t n = text.size();
	while (i < n)
	{
		const char c = text[i];
		if (c == '/' && i + 1 < n && text[i + 1] == '/')
		{
			while (i < n && text[i] != '\n')
				i++;
			continue;
		}
		if (c == '/' && i + 1 < n && text[i + 1] == '*')
		{
			i += 2;
			while (i + 1 < n && !(text[i] == '*' && text[i + 1] == '/'))
			{
				out.push_back(text[i] == '\n' ? '\n' : ' ');
				i++;
			}
			i = i + 1 < n ? i + 2 : n;
			continue;
		}
		if (c == '"' || c == '\'')
		{
			const char quote = c;
			i++;
			while (i < n && text[i] != quote)
			{
				if (text[i] == '\\')
					i++;
				if (i < n && text[i] == '\n')
					out.push_back('\n');
				i++;
			}
			i++;
			out.push_back(' ');
			continue;
		}

		out.push_back(c);
		i++;
	}
	return out;
}

// Collects the entry points and enums the frontend header declares.
static void collectFacadeSurface(const std::string &header, std::set<std::string> &functions,
	std::set<std::string> &constants)
{
	const std::string stripped = stripCommentsAndLiterals(header);

	std::size_t i = 0;
	while (i < stripped.size())
	{
		if (stripped.compare(i, 8, "#define ") == 0)
		{
			std::size_t start = i + 8;
			while (start < stripped.size() && stripped[start] == ' ')
				start++;
			std::size_t end = start;
			while (end < stripped.size() && isIdentifierChar(stripped[end]))
				end++;
			const std::string name = stripped.substr(start, end - start);
			if (name.compare(0, 3, "GL_") == 0)
				constants.insert(name);
			i = end;
			continue;
		}

		if (stripped[i] == 'g' && stripped.compare(i, 2, "gl") == 0 &&
			(i == 0 || !isIdentifierChar(stripped[i - 1])))
		{
			std::size_t end = i;
			while (end < stripped.size() && isIdentifierChar(stripped[end]))
				end++;
			std::size_t open = end;
			while (open < stripped.size() && stripped[open] == ' ')
				open++;
			if (open < stripped.size() && stripped[open] == '(' && end - i > 2)
				functions.insert(stripped.substr(i, end - i));
			i = end;
			continue;
		}

		i++;
	}
}

static void recordUsage(std::map<std::string, SymbolUsage> &table, const std::string &name,
	const std::string &file, int line)
{
	SymbolUsage &usage = table[name];
	usage.count++;
	usage.files.insert(file);
	if (usage.firstFile.empty())
	{
		usage.firstFile = file;
		usage.firstLine = line;
	}
}

static void scanSource(const std::string &text, const std::string &relativePath,
	std::map<std::string, SymbolUsage> &functions, std::map<std::string, SymbolUsage> &constants)
{
	const std::string stripped = stripCommentsAndLiterals(text);

	int line = 1;
	std::size_t i = 0;
	while (i < stripped.size())
	{
		if (stripped[i] == '\n')
		{
			line++;
			i++;
			continue;
		}

		const bool boundary = (i == 0) || !isIdentifierChar(stripped[i - 1]);
		if (!boundary)
		{
			i++;
			continue;
		}

		if (stripped.compare(i, 3, "GL_") == 0)
		{
			std::size_t end = i;
			while (end < stripped.size() && isIdentifierChar(stripped[end]))
				end++;
			recordUsage(constants, stripped.substr(i, end - i), relativePath, line);
			i = end;
			continue;
		}

		if (stripped.compare(i, 2, "gl") == 0)
		{
			std::size_t end = i;
			while (end < stripped.size() && isIdentifierChar(stripped[end]))
				end++;
			// A GL entry point is an identifier starting with gl or glu followed
			// by an upper-case letter, used as a call.
			const std::string name = stripped.substr(i, end - i);
			const bool shapedLikeEntryPoint = name.size() > 2 &&
				((name[2] >= 'A' && name[2] <= 'Z') ||
					(name.size() > 3 && name[2] == 'u' && name[3] >= 'A' && name[3] <= 'Z'));
			const bool shapedLikeGladEntryPoint = name.size() > 7 &&
				name.compare(0, 7, "glad_gl") == 0 && name[7] >= 'A' && name[7] <= 'Z';
			std::size_t open = end;
			while (open < stripped.size() && (stripped[open] == ' ' || stripped[open] == '\t'))
				open++;
			if ((shapedLikeEntryPoint || shapedLikeGladEntryPoint) &&
				open < stripped.size() && stripped[open] == '(')
				recordUsage(functions, name, relativePath, line);
			i = end;
			continue;
		}

		i++;
	}
}

static bool hasPrefix(const std::string &text, const char *prefix)
{
	return text.compare(0, std::char_traits<char>::length(prefix), prefix) == 0;
}

static bool isBackendSourcePath(const std::string &relativePath)
{
	return hasPrefix(relativePath, "backends/NativeGL/") ||
		hasPrefix(relativePath, "backends/D3D12/") ||
		hasPrefix(relativePath, "backends/OpenGL21/") ||
		hasPrefix(relativePath, "backends/OpenGL33/") ||
		hasPrefix(relativePath, "backends/OpenGL/") ||
		hasPrefix(relativePath, "backends/Vulkan/") ||
		hasPrefix(relativePath, "backends/Platform/");
}

static bool isForbiddenCoreFunction(const std::string &name)
{
	static const std::set<std::string> forbidden = {
		// Matrix stacks and fixed-function transforms.
		"glMatrixMode", "glLoadIdentity", "glLoadMatrixf", "glLoadMatrixd",
		"glMultMatrixf", "glMultMatrixd", "glPushMatrix", "glPopMatrix",
		"glTranslatef", "glTranslated", "glRotatef", "glRotated", "glScalef",
		"glScaled", "glOrtho", "glFrustum",

		// Immediate mode and legacy client arrays.
		"glBegin", "glEnd", "glArrayElement", "glEnableClientState",
		"glDisableClientState", "glVertexPointer", "glTexCoordPointer",
		"glColorPointer", "glNormalPointer", "glIndexPointer", "glEdgeFlagPointer",
		"glInterleavedArrays", "glClientActiveTexture",

		// Display lists.
		"glGenLists", "glNewList", "glEndList", "glCallList", "glCallLists",
		"glDeleteLists", "glListBase", "glIsList",

		// Fixed-function fragment and vertex processing.
		"glAlphaFunc", "glShadeModel", "glColorMaterial", "glClipPlane",
		"glGetClipPlane", "glLineStipple", "glPolygonStipple", "glGetPolygonStipple",
		"glBitmap", "glDrawPixels", "glCopyPixels", "glPixelZoom",
		"glPushAttrib", "glPopAttrib", "glPushClientAttrib", "glPopClientAttrib",
		"glAccum", "glRenderMode", "glSelectBuffer", "glFeedbackBuffer",
		"glInitNames", "glLoadName", "glPushName", "glPopName", "glPassThrough"
	};

	if (forbidden.count(name) != 0)
		return true;

	// These families contain only compatibility-profile entry points. Keep the
	// numeric check on glVertex/glColor so modern glVertexAttrib and glColorMask
	// remain available to the Core backend.
	if ((hasPrefix(name, "glVertex") && name.size() > 8 && name[8] >= '2' && name[8] <= '4') ||
		(hasPrefix(name, "glColor") && name.size() > 7 && name[7] >= '3' && name[7] <= '4') ||
		hasPrefix(name, "glTexCoord") || hasPrefix(name, "glMultiTexCoord") ||
		hasPrefix(name, "glNormal3") || hasPrefix(name, "glSecondaryColor") ||
		hasPrefix(name, "glEdgeFlag") || hasPrefix(name, "glIndex") ||
		hasPrefix(name, "glRect") || hasPrefix(name, "glRasterPos") ||
		hasPrefix(name, "glWindowPos") || hasPrefix(name, "glLight") ||
		hasPrefix(name, "glGetLight") || hasPrefix(name, "glMaterial") ||
		hasPrefix(name, "glGetMaterial") || hasPrefix(name, "glFog") ||
		hasPrefix(name, "glTexEnv") || hasPrefix(name, "glGetTexEnv") ||
		hasPrefix(name, "glTexGen") || hasPrefix(name, "glGetTexGen") ||
		hasPrefix(name, "glEvalCoord") || hasPrefix(name, "glEvalMesh") ||
		hasPrefix(name, "glEvalPoint") || hasPrefix(name, "glMapGrid") ||
		hasPrefix(name, "glMap1") || hasPrefix(name, "glMap2"))
		return true;

	// The Core backend must use the unsuffixed 4.6 entry points rather than
	// compatibility-era ARB aliases such as glBindBufferARB.
	return name.size() > 3 && name.compare(name.size() - 3, 3, "ARB") == 0;
}

int main(int argc, char **argv)
{
	std::filesystem::path root = A126_SOURCE_ROOT;
	bool printTable = false;
	for (int i = 1; i < argc; i++)
	{
		const std::string argument = argv[i];
		if (argument == "--table")
			printTable = true;
		else
			root = argument;
	}

	const std::filesystem::path facadePath = root / "legacygl" / "LegacyGL.h";
	const std::string facade = readFile(facadePath);
	if (facade.empty())
	{
		std::cerr << "gl-inventory: cannot read " << facadePath.string() << '\n';
		return 2;
	}

	std::set<std::string> declaredFunctions;
	std::set<std::string> declaredConstants;
	collectFacadeSurface(facade, declaredFunctions, declaredConstants);

	// Helpers the project itself provides on top of the frontend.
	const std::set<std::string> projectHelpers = { "gluPerspective", "gluFrustum" };

	std::map<std::string, SymbolUsage> usedFunctions;
	std::map<std::string, SymbolUsage> usedConstants;
	std::vector<std::string> problems;
	int filesScanned = 0;

	for (std::filesystem::recursive_directory_iterator it(root), end; it != end; ++it)
	{
		if (it->is_directory())
		{
			const std::string name = it->path().filename().string();
			if (name == "external")
				it.disable_recursion_pending();
			continue;
		}
		if (!it->is_regular_file())
			continue;

		const std::string extension = it->path().extension().string();
		if (extension != ".cpp" && extension != ".h")
			continue;

		const std::string relative = std::filesystem::relative(it->path(), root).generic_string();
		// The frontend's own declaration and definition files are the surface,
		// not users of it. Skipping them keeps the "declared but never called"
		// report meaningful.
		if (relative == "legacygl/LegacyGL.h" || relative == "legacygl/Facade.cpp" ||
			isBackendSourcePath(relative))
			continue;

		filesScanned++;
		scanSource(readFile(it->path()), relative, usedFunctions, usedConstants);
	}

	// Backends are below the facade boundary, so their implementation calls
	// must not expand the frontend inventory. The GL2.1 compatibility lowerer
	// deliberately uses fixed-function calls; Core OpenGL may use modern GL
	// only, and Vulkan/D3D12 may not call OpenGL at all.
	static const char *coreBackendDirectories[] = { "OpenGL33", "OpenGL", "Vulkan", "D3D12" };
	for (const char *directory : coreBackendDirectories)
	{
		const std::filesystem::path coreRoot = root / "backends" / directory;
		if (!std::filesystem::is_directory(coreRoot))
			continue;

		std::map<std::string, SymbolUsage> coreFunctions;
		std::map<std::string, SymbolUsage> coreConstants;
		for (std::filesystem::recursive_directory_iterator it(coreRoot), end; it != end; ++it)
		{
			if (!it->is_regular_file())
				continue;
			const std::string extension = it->path().extension().string();
			if (extension != ".cpp" && extension != ".h")
				continue;

			const std::string relative = std::filesystem::relative(it->path(), root).generic_string();
			scanSource(readFile(it->path()), relative, coreFunctions, coreConstants);
		}
		for (const std::pair<const std::string, SymbolUsage> &entry : coreFunctions)
		{
			const std::string classifiedName = hasPrefix(entry.first, "glad_gl")
				? entry.first.substr(5) : entry.first;
			const bool zeroOpenGLBackend = std::string(directory) == "Vulkan" ||
				std::string(directory) == "D3D12";
			if (!zeroOpenGLBackend && !isForbiddenCoreFunction(classifiedName))
				continue;
			std::ostringstream problem;
			problem << entry.first << " is forbidden in " << entry.second.firstFile << ':'
					<< entry.second.firstLine;
			if (zeroOpenGLBackend)
				problem << "; the " << directory << " backend may not call OpenGL";
			else
				problem << "; the Core backend may call modern GL only";
			problems.push_back(problem.str());
		}
	}

	for (const std::pair<const std::string, SymbolUsage> &entry : usedFunctions)
	{
		if (declaredFunctions.count(entry.first) != 0 || projectHelpers.count(entry.first) != 0)
			continue;
		std::ostringstream problem;
		problem << entry.first << " is called at " << entry.second.firstFile << ':' << entry.second.firstLine
				<< " but the LegacyGL frontend does not declare it";
		problems.push_back(problem.str());
	}

	for (const std::pair<const std::string, SymbolUsage> &entry : usedConstants)
	{
		if (declaredConstants.count(entry.first) != 0)
			continue;
		std::ostringstream problem;
		problem << entry.first << " is used at " << entry.second.firstFile << ':' << entry.second.firstLine
				<< " but the LegacyGL frontend does not define it";
		problems.push_back(problem.str());
	}

	std::cout << "gl-inventory: scanned " << filesScanned << " files under " << root.generic_string() << '\n';
	std::cout << "gl-inventory: " << usedFunctions.size() << " entry points and " << usedConstants.size()
			  << " enums used; frontend declares " << declaredFunctions.size() << " entry points and "
			  << declaredConstants.size() << " enums\n";

	if (printTable)
	{
		std::cout << "\n| function | call sites | files | first use |\n";
		std::cout << "|---|---|---|---|\n";
		for (const std::pair<const std::string, SymbolUsage> &entry : usedFunctions)
		{
			std::cout << "| `" << entry.first << "` | " << entry.second.count << " | "
					  << entry.second.files.size() << " | " << entry.second.firstFile << ':'
					  << entry.second.firstLine << " |\n";
		}

		std::cout << "\n| enum | uses | files |\n";
		std::cout << "|---|---|---|\n";
		for (const std::pair<const std::string, SymbolUsage> &entry : usedConstants)
		{
			std::cout << "| `" << entry.first << "` | " << entry.second.count << " | "
					  << entry.second.files.size() << " |\n";
		}
	}

	// Declared but unused entries are reported too: the frontend surface should
	// not grow beyond what the game issues.
	for (const std::string &declared : declaredFunctions)
	{
		if (usedFunctions.count(declared) == 0)
			std::cout << "gl-inventory: note: " << declared << " is declared but never called\n";
	}

	if (!problems.empty())
	{
		std::cerr << "\ngl-inventory: " << problems.size() << " coverage failures\n";
		for (const std::string &problem : problems)
			std::cerr << "  " << problem << '\n';
		std::cerr << "\nAdd an explicit semantic implementation before using a new GL entry point or enum.\n";
		return 1;
	}

	std::cout << "gl-inventory: every GL symbol the source uses is covered by the frontend\n";
	return 0;
}
