/** Authoring model: links are first class; no authored junction/node graph. */
export interface Point { readonly x: number; readonly y: number }
export interface Lane { readonly id: string; readonly width: number }
export interface Link {
  readonly id: string;
  readonly geometry: readonly Point[];
  /** Ordered from the driving-side curb inward. IDs are globally unique. */
  readonly lanes: readonly Lane[];
}
export interface LaneReference { readonly linkId: string; readonly laneId: string }
export interface Connector {
  readonly id: string;
  readonly from: LaneReference;
  readonly to: LaneReference;
  /** Lane-centre geometry from the upstream lane end to downstream lane start. */
  readonly geometry: readonly Point[];
}
export interface NetworkSignalHead {
  readonly id: string;
  readonly lane: LaneReference;
  readonly position: number;
  readonly programId: string;
}
export interface Network {
  readonly id: string;
  readonly drivingSide: 'left' | 'right';
  readonly links: readonly Link[];
  readonly connectors: readonly Connector[];
  readonly signalHeads: readonly NetworkSignalHead[];
}
export interface NetworkIssue { readonly code: string; readonly path: string }
