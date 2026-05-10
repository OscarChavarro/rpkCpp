import { ColorRgb } from "./color/ColorRgb";

export class RenderOptions {
  public outlineColor: ColorRgb;
  public boundingBoxColor: ColorRgb;
  public clusterColor: ColorRgb;
  public lineWidth: number;
  public drawOutlines: boolean;
  public drawSurfaces: boolean;
  public noShading: boolean;
  public smoothShading: boolean;
  public backfaceCulling: boolean;
  public drawBoundingBoxes: boolean;
  public drawClusters: boolean;
  public frustumCulling: boolean;
  public renderRayTracedImage: boolean;
  public trace: boolean;

  private static readonly DEFAULT_SMOOTH_SHADING = true;
  private static readonly DEFAULT_BACKFACE_CULLING = true;
  private static readonly DEFAULT_OUTLINE_DRAWING = false;
  private static readonly DEFAULT_SURFACE_DRAWING = true;
  private static readonly DEFAULT_BOUNDING_BOX_DRAWING = false;
  private static readonly DEFAULT_CLUSTER_DRAWING = false;
  private static readonly DEFAULT_OUTLINE_COLOR = new ColorRgb(0.5, 0.0, 0.0);
  private static readonly DEFAULT_BOUNDING_BOX_COLOR = new ColorRgb(0.5, 0.0, 1.0);
  private static readonly DEFAULT_CLUSTER_COLOR = new ColorRgb(1.0, 0.5, 0.0);

  public constructor() {
    this.outlineColor = new ColorRgb();
    this.boundingBoxColor = new ColorRgb();
    this.clusterColor = new ColorRgb();

    this.smoothShading = RenderOptions.DEFAULT_SMOOTH_SHADING;
    this.backfaceCulling = RenderOptions.DEFAULT_BACKFACE_CULLING;
    this.drawSurfaces = RenderOptions.DEFAULT_SURFACE_DRAWING;
    this.drawOutlines = RenderOptions.DEFAULT_OUTLINE_DRAWING;
    this.drawBoundingBoxes = RenderOptions.DEFAULT_BOUNDING_BOX_DRAWING;
    this.drawClusters = RenderOptions.DEFAULT_CLUSTER_DRAWING;

    this.outlineColor = new ColorRgb(
      RenderOptions.DEFAULT_OUTLINE_COLOR.r,
      RenderOptions.DEFAULT_OUTLINE_COLOR.g,
      RenderOptions.DEFAULT_OUTLINE_COLOR.b
    );
    this.boundingBoxColor = new ColorRgb(
      RenderOptions.DEFAULT_BOUNDING_BOX_COLOR.r,
      RenderOptions.DEFAULT_BOUNDING_BOX_COLOR.g,
      RenderOptions.DEFAULT_BOUNDING_BOX_COLOR.b
    );
    this.clusterColor = new ColorRgb(
      RenderOptions.DEFAULT_CLUSTER_COLOR.r,
      RenderOptions.DEFAULT_CLUSTER_COLOR.g,
      RenderOptions.DEFAULT_CLUSTER_COLOR.b
    );

    this.frustumCulling = false;
    this.noShading = false;
    this.lineWidth = 1.0;
    this.renderRayTracedImage = false;
    this.trace = false;
  }
}
