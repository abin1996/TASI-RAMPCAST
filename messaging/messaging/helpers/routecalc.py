#Reference :https://github.com/gboeing/osmnx-examples/blob/main/notebooks/

# import the modules
import osmnx as ox
import networkx as nx
import os

# define a class for the graph
class OpenStreetGraph:
    # use the __init__ method to initialize the graph object
    def __init__(self, filepath, centre_point=(39.782214, -86.167552), radius=100000):
        self.filepath = filepath # the filepath for the XML file
        self.centre_point = centre_point
        self.radius = radius
        self.load_route_graph()

    # define a method to get the graph from the XML file or from the point
    def load_route_graph(self):
        # check if the file exists
        if os.path.exists(self.filepath):
            # load the graph from the XML file
            self.G = ox.io.load_graphml(self.filepath)
        else:
            # get the graph within 10 km of the point
            #self.G = ox.graph_from_point(self.centre_point, dist=self.radius, network_type='drive')
            cf = '["highway"~"primary|motorway"]'

            self.G = ox.graph_from_place("Indiana, USA", network_type="drive",custom_filter=cf)

            #self.G = ox.graph_from_place("Indiana, USA", network_type="drive")
            # save the graph as an XML file
            ox.io.save_graphml(self.G, filepath=self.filepath)

    # define a method to calculate the shortest path distance between the two points
    def shortest_road_distance(self, origin,
                               dest):

        # get the nearest edges and distances from the coordinates
        (u1, v1, k1), d1 = ox.nearest_edges(self.G, X=origin[1], Y=origin[0], return_dist=True)
        (u2, v2, k2), d2 = ox.nearest_edges(self.G, X=dest[1], Y=dest[0], return_dist=True)

        # get the edge IDs from the edge attributes
        edge1 = (u1, v1, k1)
        edge2 = (u2, v2, k2)

        # check if the edges are in the graph and connected
        if self.G.has_edge(*edge1) and self.G.has_edge(*edge2) and nx.has_path(self.G, u1, u2):
            # calculate the shortest path between the two edges
            self.route = nx.shortest_path(self.G, u1, u2, weight='length')
            # calculate the length of the shortest path
            gdf = ox.utils_graph.route_to_gdf(self.G, self.route, weight='length')
            self.distance = gdf['length'].sum() + d1 + d2

            #self.visualize_path(origin, dest, self.G, self.route)

            return self.distance * 0.000621371
        else:
            # handle the case where there is no path
            print("No path found between the edges")
            return 0

    def visualize_path(self, origin, dest, G, route):
        # create a bbox from the midpoint of the route and a distance of 500 meters
        midpoint = ((origin[0] + dest[0]) / 2, (origin[1] + dest[1]) / 2)
        bbox = ox.utils_geo.bbox_from_point(midpoint, dist=self.distance)

        # visualize the path with the bbox
        fig, ax = ox.plot_graph_route(G, route, node_size=0, edge_color='gray', edge_linewidth=0.5,
                                      route_color='green',
                                      route_linewidth=2, show=False, bbox=bbox)
        fig.show()

# # create an instance of the graph class with the given filepath
#graph = OpenStreetGraph(filepath='indiana_route.graphml')
#
# # call the methods of the graph object
#distance = graph.shortest_road_distance((39.781440, -86.167783), (39.78267361,-86.14946682))
# print("The distance1 is", distance, "miles")
#
#
# distance = graph.shortest_road_distance((39.782049, -86.167675), (39.783729, -86.166158))
# print("The distance2 is", distance, "miles")
#
#
# distance = graph.shortest_road_distance((39.782049, -86.167675), (39.793245, -86.164391))
# print("The distance3 is", distance, "miles")

