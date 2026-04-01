#ifndef __STOCHASTIC_RADIOSITY_OPENGL_RENDERER__
#define __STOCHASTIC_RADIOSITY_OPENGL_RENDERER__

class Element;
class RenderOptions;
class StochasticRadiosityElement;
class Vertex;

class StochasticRadiosityOpenGLRenderer {
  public:
    static void renderElement(Element *element, const RenderOptions *renderOptions);

  private:
    static void renderTriangle(const Vertex *v1, const Vertex *v2, const Vertex *v3, const RenderOptions *renderOptions);
    static void renderQuadrilateral(
        const Vertex *v1,
        const Vertex *v2,
        const Vertex *v3,
        const Vertex *v4,
        const RenderOptions *renderOptions);
    static void triangleTVertexElimination(
        Vertex **vertices,
        Vertex **midpoints,
        int numberOfTVertices,
        void (*doTriangleCallback)(const Vertex *, const Vertex *, const Vertex *, const RenderOptions *),
        const RenderOptions *renderOptions);
    static void quadrilateralTVertexElimination(
        Vertex **vertices,
        Vertex **midpoints,
        int numberOfTVertices,
        void (*doTriangleCallback)(const Vertex *, const Vertex *, const Vertex *, const RenderOptions *),
        void (*doQuadrilateralCallback)(const Vertex *, const Vertex *, const Vertex *, const Vertex *, const RenderOptions *),
        const RenderOptions *renderOptions);
    static void renderTriangularElement(Vertex **vertices, Vertex **midpoints, int numberOfTVertices, const RenderOptions *renderOptions);
    static void renderQuadrilateralElement(Vertex **vertices, Vertex **midpoints, int numberOfTVertices, const RenderOptions *renderOptions);
    static void renderOutline(const StochasticRadiosityElement *element, const RenderOptions *renderOptions);
};

#endif
