// Determines how source radiance of a source cluster is determined and
// how irradiance is distributed over the patches in a receiver cluster
export enum GalerkinClusteringStrategy {
  ISOTROPIC,
  ORIENTED,
  Z_VISIBILITY,
}
