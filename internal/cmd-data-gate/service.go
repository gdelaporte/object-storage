//
// Copyright 2019-2020 Jean-Francois Smigielski
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt). A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

package cmd_data_gate

import (
	"github.com/jfsmig/object-storage/pkg/gunkan"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promauto"
	"math"
	"time"
)

type config struct {
	uuid         string
	addrBind     string
	addrAnnounce string
	dirConfig    string
}

type service struct {
	config config

	lb gunkan.Balancer

	timePut  prometheus.Histogram
	timeGet  prometheus.Histogram
	timeDel  prometheus.Histogram
	timeList prometheus.Histogram
}

func newService(cfg config) (*service, error) {
	var err error
	srv := service{config: cfg}
	srv.lb, err = gunkan.NewBalancerDefault()

	buckets := []float64{0.01, 0.02, 0.03, 0.04, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5, 1, 2, 3, 4, 5, math.Inf(1)}

	srv.timeList = promauto.NewHistogram(prometheus.HistogramOpts{
		Name:    "gunkan_part_list_ttlb",
		Help:    "Repartition of the request times of List requests",
		Buckets: buckets,
	})

	srv.timePut = promauto.NewHistogram(prometheus.HistogramOpts{
		Name:    "gunkan_part_put_ttlb",
		Help:    "Repartition of the request times of put requests",
		Buckets: buckets,
	})

	srv.timeGet = promauto.NewHistogram(prometheus.HistogramOpts{
		Name:    "gunkan_part_get_ttlb",
		Help:    "Repartition of the request times of get requests",
		Buckets: buckets,
	})

	srv.timeDel = promauto.NewHistogram(prometheus.HistogramOpts{
		Name:    "gunkan_part_del_ttlb",
		Help:    "Repartition of the request times of del requests",
		Buckets: buckets,
	})

	if err != nil {
		return nil, err
	} else {
		return &srv, nil
	}
}

func (srv *service) isOverloaded(now time.Time) bool {
	return false
}
